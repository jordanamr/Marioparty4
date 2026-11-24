using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
namespace PartyBoardOnline {
// Structured diagnostics only: never invitation, endpoint, nickname, disk path
// or arbitrary engine/exception output.
sealed class Report {
    public readonly string DirectoryPath;
    public string NativePath {get{return Path.Combine(DirectoryPath,"native.txt");}}
    readonly object gate=new object();int lines;
    static string Root {get{return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData,Environment.SpecialFolderOption.DoNotVerify),"PartyBoard","Diagnostics");}}
    public Report(string root=null) {
        DirectoryPath=Path.Combine(root??Root,DateTime.UtcNow.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N"));
        try{Directory.CreateDirectory(DirectoryPath);Write("format=1");}catch{}
    }
    public void Write(string text) { Write(text,false); }
    // Reserve space for the exit code and cause even in a long game.
    public void Write(string text,bool terminal) {
        lock(gate){if(lines>=(terminal?3008:3000))return;try{File.AppendAllText(Path.Combine(DirectoryPath,"session.txt"),DateTime.UtcNow.ToString("O")+" "+text+Environment.NewLine,Encoding.UTF8);lines++;}catch{}}
    }
    // The engine records the exact report it produced in this session's sidecar.
    // Resolve only its generated filename inside the game's report directory.
    static readonly Regex ReportName=new Regex(@"(?:^|[/\\])(netplay_desync_[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{6}_player[1-4]\.log)\z");
    static string DefaultDesyncFolder() {
        string data=Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
        if(Environment.OSVersion.Platform==PlatformID.Unix) {
            data=Environment.GetEnvironmentVariable("XDG_DATA_HOME");
            if(String.IsNullOrEmpty(data))data=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),".local","share");
        }
        return Path.Combine(data,"MarioPartyRD","Party Board","netplay");
    }
    static string SessionDesyncReport(string sidecar,string folder) {
        try {
            if(!File.Exists(sidecar))return null;
            string line=File.ReadLines(sidecar).FirstOrDefault();
            if(line==null)return null;
            int marker=line.IndexOf(" report=",StringComparison.Ordinal);
            if(marker<0)return null;
            var match=ReportName.Match(line.Substring(marker+8));
            if(!match.Success)return null;
            var path=Path.Combine(folder,match.Groups[1].Value);
            return File.Exists(path)?path:null;
        } catch(IOException){return null;} catch(UnauthorizedAccessException){return null;}
    }

    // Reads at most `budget` characters. `fromTail` decides which end survives
    // when the file is bigger: the running trace is worth reading backwards
    // from the moment it stopped, a desync report forwards from its header.
    static void AppendSection(StringBuilder result,string title,string path,bool fromTail,int budget) {
        if(result.Length>0 && result[result.Length-1]!=(char)10)result.AppendLine();
        result.AppendLine("--- "+title+" ---");
        if(path==null){result.AppendLine("No data available.");return;}
        try{using(var f=new FileStream(path,FileMode.Open,FileAccess.Read,FileShare.ReadWrite)){
            if(fromTail && f.Length>budget){
                f.Seek(f.Length-budget,SeekOrigin.Begin);
                result.AppendLine("[debut tronque : "+(f.Length-budget)+" octets plus anciens omis]");
            }
            using(var reader=new StreamReader(f)){
                var buffer=new char[budget];int n=reader.ReadBlock(buffer,0,buffer.Length);
                result.Append(buffer,0,n);
                if(!fromTail && reader.Peek()>=0)
                    {result.AppendLine();result.AppendLine("[fin tronquee]");}
            }
        }}catch(IOException){result.AppendLine("No data available.");}
        catch(UnauthorizedAccessException){result.AppendLine("No data available.");}
    }

    public string Read() { return Read(DefaultDesyncFolder()); }
    internal string Read(string desyncFolder) {
        var result=new StringBuilder("PartyBoard diagnostic v1\r\n");
        AppendSection(result,"session.txt",Path.Combine(DirectoryPath,"session.txt"),false,1024*1024);
        AppendSection(result,"native.txt",Path.Combine(DirectoryPath,"native.txt"),true,1024*1024);
        AppendSection(result,"native.txt.desync",Path.Combine(DirectoryPath,"native.txt.desync"),false,256*1024);
        var desync=SessionDesyncReport(NativePath+".desync",desyncFolder);
        AppendSection(result,desync==null?"netplay_desync (absent)":Path.GetFileName(desync),
            desync,false,2*1024*1024);
        return result.ToString();
    }
    public static string ReadLatest() {
        if(!Directory.Exists(Root))throw new IOException("Créez ou rejoignez un salon avant d'exporter le diagnostic.");
        var path=Directory.GetDirectories(Root).OrderByDescending(p=>p,StringComparer.Ordinal).FirstOrDefault();
        if(path==null)throw new IOException("Aucun diagnostic disponible.");
        return new Report(path,true).Read();
    }
    Report(string path,bool existing){DirectoryPath=path;}
}
}
