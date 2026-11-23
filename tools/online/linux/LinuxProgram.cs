using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace PartyBoardOnline {

// The native game's lobby window owns this process and its private state directory.
// The networking and launch coordinator below the window are shared with Windows.
sealed class LinuxRoom {
    readonly Startup startup;
    readonly CancellationTokenSource cancel = new CancellationTokenSource();
    volatile string message;
    volatile string failure;
    volatile Session session;
    volatile bool opened;
    bool closing;
    string lastSnapshot;
    long revision;

    public LinuxRoom(Startup startup) { this.startup=startup; }

    void Publish(string state, string invitation=null) {
        var current=session;
        var lobby=current?.Lobby;
        bool canStart=state=="waiting" && lobby?.CanStart==true && current.GameRouteReady;
        string detail=failure??message??"";
        if(state=="closed") detail=L.T("Le salon est fermé.");
        else if(state=="preparing") detail=L.T("Chargement sur les deux PC… Le jeu attendra que tout le monde soit prêt. Aucun bouton à presser dans l'autre fenêtre.");
        else if(state=="playing") detail=L.T("Partie lancée par l'hôte. Gardez le salon ouvert pendant le jeu.");
        else if(state=="waiting" && lobby?.Occupied>=2) {
            if(!lobby.DiscMatches)detail=L.T("Tous les joueurs doivent avoir exactement le meme fichier disque.");
            else if(!lobby.ModsMatch)detail=L.T("Tous les joueurs doivent avoir exactement les memes mods actifs : ")+lobby.ModAdvice;
            else if(canStart)detail=L.T("Disques et mods identiques. Vous pouvez lancer la partie pour tout le monde.");
        }
        var fields=new Dictionary<string,object>{
            {"state",state}, {"message",detail},
            {"invitation",invitation??""}, {"canStart",canStart},
            {"players",current?.RoomStatus().Players??new List<string>()}
        };
        var serialized=JsonSerializer.Serialize(fields);
        if(serialized==lastSnapshot)return;
        RoomSync.Write(startup.StateDirectory,fields,++revision);
        lastSnapshot=serialized;
    }

    void Open() {
        try {
            using(var disc=DiscFile.Verify(startup.DiscPath,p=>message=L.T("Vérification du disque")+" "+p+" %",cancel.Token)) {
                var mods=ModSet.FromCubeShelf("GMPE01_00",out _);
                if(startup.ModIds!=null) {
                    var wanted=new ModSet(startup.ModIds.Select(id=>new ModEntry(id,"","",new byte[8])));
                    mods=ModSet.AlignedTo(wanted,InstalledMod.FromCubeShelf("GMPE01_00",out _));
                }
                var profile=new PlayerInfo(startup.Nickname??"Player",disc.Hash,disc.Length,mods,GameLanguage.Current);
                using(var current=new Session(m=>message=m,()=>{},err=>failure=err,profile,disc)) {
                    session=current;
                    cancel.Token.ThrowIfCancellationRequested();
                    if(startup.Mode==StartupMode.Host) {
                        current.RoomName=startup.RoomName;
                        current.Create(startup.Players??2);
                        message=current.LocalOnly && !current.RelayReady
                            ? L.T("Cette invitation fonctionne sur le réseau local. Pour jouer par Internet, vérifiez le relais ou la redirection de port, puis recréez le salon.")
                            : L.T("En attente de votre ami…");
                    } else {
                        current.Join(startup.Invitation);
                        message=L.T("Attendez que l'hôte lance la partie.");
                    }
                    opened=true;
                    cancel.Token.WaitHandle.WaitOne();
                }
            }
        } catch(OperationCanceledException) when(cancel.IsCancellationRequested) {
        } catch(Exception error) { failure=error.GetBaseException().Message; }
    }

    static Process ParentProcess() {
        var value=Environment.GetEnvironmentVariable("PARTYBOARD_ONLINE_PARENT_PID");
        return int.TryParse(value,out var pid)?Process.GetProcessById(pid):null;
    }

    public int Run() {
        using(var parent=ParentProcess()) {
            Publish("opening");
            var worker=Task.Run(Open);
            try {
                while(!closing && parent?.HasExited!=true) {
                    var current=session;
                    var lobby=current?.Lobby;
                    if(failure!=null || lobby?.Phase==LobbyPhase.Closed) {
                        bool gameEnded=lobby?.Ending==LobbyEnding.LocalGameClosed || lobby?.Ending==LobbyEnding.RemoteGameClosed;
                        Publish(gameEnded?"closed":"error");
                        return gameEnded?0:1;
                    }
                    string state=!opened?"opening":lobby?.Phase==LobbyPhase.Preparing?"preparing":
                        lobby?.Phase==LobbyPhase.Running?"playing":"waiting";
                    string invitation=null;
                    if(opened && current.Host) {
                        // A stock Windows client treats a private-IP invitation as LAN only.
                        // When that is our only address, give it the existing relay-only form.
                        var invite=current.LocalOnly && current.RelayReady?current.Invite.ForPublicList():current.Invite;
                        invitation=invite.Encode();
                    }
                    Publish(state,invitation);
                    foreach(var request in RoomSync.Read(startup.StateDirectory,DateTime.UtcNow)) {
                        try {
                            if(request.Action=="close") closing=true;
                            else if(request.Action=="start" && current?.Host==true)current.Launch();
                            else throw new IOException(L.T("La partie ne peut pas encore être lancée."));
                            RoomSync.Complete(startup.StateDirectory,request,true,"");
                        } catch(Exception error) {
                            message=error.Message;
                            RoomSync.Complete(startup.StateDirectory,request,false,message);
                        }
                    }
                    cancel.Token.WaitHandle.WaitOne(100);
                }
                Publish("closed");
                return 0;
            } finally {
                var current=session;
                current?.Lobby?.LocalGameExited();
                cancel.Cancel();
                // A close request can arrive while mapping ports or preparing a game.
                current?.Dispose();
                worker.Wait(TimeSpan.FromSeconds(5));
                var child=current?.Game;
                if(child!=null && !child.HasExited && !child.WaitForExit(2000))
                    child.Kill(entireProcessTree:true);
            }
        }
    }
}

static class LinuxProgram {
    static int Main(string[] args) {
        L.Init(Environment.GetEnvironmentVariable);
        GameLanguage.Current=GameLanguage.FromArguments(args);
        if(Array.IndexOf(args,"--game-language")>=0 && string.IsNullOrEmpty(Environment.GetEnvironmentVariable(L.Variable)))
            L.Language=GameLanguage.Code(GameLanguage.Current);
        try {
            if(args.Length==1 && args[0]=="--build-hash") {
                Console.WriteLine(Convert.ToHexString(Wire.BuildHash(AppContext.BaseDirectory)).ToLowerInvariant());
                return 0;
            }
            if(args.Length==1 && args[0]=="--verify-disc") {
                using(var disc=DiscFile.Verify(Environment.GetEnvironmentVariable(Startup.DiscVariable),_=>{},CancellationToken.None))
                    Console.WriteLine((disc.Pal?"GMPP01":"GMPE01")+" "+Convert.ToHexString(disc.Hash).ToLowerInvariant());
                return 0;
            }
            var startup=Startup.FromEnvironment(args);
            if(startup.Mode==StartupMode.Manual || startup.StateDirectory==null || startup.DiscPath==null)
                throw new ArgumentException("Open the online lobby from PartyBoard, or specify --host/--join, the disc, and a private state directory.");
            return new LinuxRoom(startup).Run();
        } catch(Exception error) {
            Console.Error.WriteLine(error.GetBaseException().Message);
            return 1;
        }
    }
}
}
