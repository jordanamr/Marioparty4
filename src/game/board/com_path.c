#include "game/board/com_path.h"
#include "game/board/space.h"
#include "game/flag.h"

#include <string.h>

#define NODE_INDEX(x) ((x) ? ((x) - pathNodeData) : -1)

typedef struct {
    /* 0x00 */ struct {
        u8 used : 1;
        u8 star : 1;
        u8 boo : 1;
    };
    /* 0x01 */ s8 num_links;
    /* 0x02 */ s8 num_children;
    /* 0x03 */ s8 max_len;
    /* 0x04 */ s16 start;
    /* 0x06 */ s16 children[32];
    /* 0x46 */ s16 links[BOARD_SPACE_LINKMAX+1];
    u32 space_flags;
} PathNode;

static PathNode *SearchPathNodeSpace(s16 arg0);
static s16 InitPathNode(PathNode *arg0);
static PathNode *FindChildNode(s16 arg0, PathNode *arg1);
static s32 GetNumValidLinks(BoardSpace *arg0, PathNode *arg1);
static void AddValidLinks(BoardSpace *arg0, PathNode *arg1);
static BOOL CheckPathSpace(BoardSpace *arg0);
static s16 FindValidLink(BoardSpace *arg0);
static BOOL CheckEndSpace(BoardSpace *arg0, PathNode *arg1);
static BOOL PathContainsTarget(PathNode *node, u32 flag);
static BOOL CanReachTarget(PathNode *node, u32 flag, u32 visited);

static PathNode pathNodeData[16];
static PathNode *candidateNodes[16];

static s16 numCandidates;
static s16 childNodeCnt;

static PathNode *SearchPathNodeSpace(s16 space) {
    PathNode *node;
    s32 i;

    for (node = pathNodeData, i = 0; i < 16; i++, node++) {
        if (node->used && node->start == space) {
            break;
        }
    }
    if (i == 16) {
        node = NULL;
    }
    return node;
}

static s16 InitPathNode(PathNode *node) {
    BoardSpace *space_ptr;
    PathNode *child;
    s32 done;
    s16 i;
    s16 space;
    s16 num_links;
    s16 path_len;
    s16 space_link;

    space_link = node->start;
    done = 0;
    node->num_children = path_len = 0;
    do {
        space = space_link;
        space_ptr = BoardSpaceGet(0, space);
        if (node->num_children == sizeof(node->children) / sizeof(node->children[0])) {
            // A run of non-counting spaces can be longer than one path segment.
            node->num_links = 1;
            node->links[0] = space;
            return node->max_len - path_len;
        }
        node->children[node->num_children] = space;
        child = FindChildNode(space, node);
        if (child) {
            AddValidLinks(space_ptr, node);
            node->num_links = 1;
            node->links[0] = space;
            for (i = 1; i < 5; i++) {
                node->links[i] = 0;
            }
            return 0;
        }
        if (space_ptr->link_cnt == 0) {
            num_links = 0;
            done = 1;
        } else {
            num_links = GetNumValidLinks(space_ptr, node);
            if (num_links == 1) {
                space_link = FindValidLink(space_ptr);
            } else {
                done = 1;
            }
        }
        if (CheckEndSpace(space_ptr, node)) {
            path_len++;
            if (path_len >= node->max_len) {
                num_links = 0;
                done = 1;
            }
        }
        node->num_children++;
    } while (done == 0);
    AddValidLinks(space_ptr, node);
    node->num_links = num_links;
    return node->max_len - path_len;
}

static PathNode *FindChildNode(s16 arg0, PathNode *arg1) {
    PathNode *node;
    s32 node_idx;
    s32 i;
    s32 j;

    node_idx = NODE_INDEX(arg1);
    if (node_idx == -1) {
        return NULL;
    }
    for (i = 0; i < childNodeCnt; i++) {
        node = &pathNodeData[i];
        if (node != arg1) {
            for (j = 0; j < node->num_children; j++) {
                if (arg0 == node->children[j]) {
                    return node;
                }
            }
        }
    }
    return NULL;
}

static s32 GetNumValidLinks(BoardSpace *space, PathNode *node) {
    BoardSpace *link_space;
    s16 i;
    s16 invalid_links;

    for (invalid_links = i = 0; i < space->link_cnt; i++) {
        link_space = BoardSpaceGet(0, space->link[i]);
        if (!CheckPathSpace(link_space)) {
            invalid_links++;
        }
    }
    if (invalid_links >= space->link_cnt) {
        return 0;
    }
    return space->link_cnt - invalid_links;
}

static void AddValidLinks(BoardSpace *space, PathNode *node) {
    BoardSpace *link_space;
    s16 i;
    s16 link;

    for (link = i = 0; i < space->link_cnt; i++) {
        link_space = BoardSpaceGet(0, space->link[i]);
        if (CheckPathSpace(link_space)) {
            node->links[link] = space->link[i];
            link++;
        }
    }
    for (; link < BOARD_SPACE_LINKMAX+1; link++) {
        node->links[link] = 0;
    }
}

static BOOL CheckPathSpace(BoardSpace *space) {
    if ((space->flag & 0x02000000) || (space->flag & 0x04000000)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static s16 FindValidLink(BoardSpace *space) {
    s16 space_link;
    s16 i;
    BoardSpace *space_ptr;

    for (space_link = i = 0; i < space->link_cnt; i++) {
        space_link = space->link[i];
        space_ptr = BoardSpaceGet(0, space_link);
        if (CheckPathSpace(space_ptr)) {
            break;
        }
    }
    return space_link;
}

static BOOL CheckEndSpace(BoardSpace *space, PathNode *node) {
    node->space_flags |= space->flag;
    if (space->flag & 0x48000000) {
        node->boo = 1;
    }
    switch (space->type) {
        case 8:
            node->star = 1;
            return FALSE;
        case 10:
            return FALSE;
        case 0:
            return FALSE;
    }
    return TRUE;
}

static inline void PushCandidate(PathNode *node) {
    if (numCandidates < 16) {
        candidateNodes[numCandidates] = node;
        numCandidates++;
    }
}

static inline PathNode *PopCandidate(void) {
    PathNode *node;

    numCandidates--;
    if (numCandidates < 0) {
        return NULL;
    } else {
        node = candidateNodes[numCandidates];
        candidateNodes[numCandidates] = NULL;
        return node;
    }
}

static inline PathNode *CreateNode(s16 start, s16 len) {
    PathNode *node;
    s32 i;

    node = pathNodeData;
    for (i = 0; i < 16; i++, node++) {
        if (node->used == 0) {
            break;
        }
    }
    if (i == 16) {
        return NULL;
    } else {
        node->used = 1;
        node->start = start;
        node->max_len = len;
        childNodeCnt++;
        return node;
    }
}

static void PopulateCandidates(s16 start, s16 len) {
    memset(pathNodeData, 0, sizeof(pathNodeData));
    numCandidates = 0;
    childNodeCnt = 0;
    PushCandidate(CreateNode(start, len));
    while (numCandidates > 0) {
        PathNode *node = PopCandidate();
        s16 remaining = InitPathNode(node);
        if (remaining > 0) {
            for (s32 i = 0; i < node->num_links; i++) {
                if (!SearchPathNodeSpace(node->links[i])) {
                    PathNode *next = CreateNode(node->links[i], remaining);
                    if (next) {
                        PushCandidate(next);
                    }
                }
            }
        }
    }
}

/* Each link costs one step for the distance queries. Search the board itself:
 * different branches can meet in the middle of a previously visited segment. */
static s16 ShortestPath(s16 start, s16 target_space, u32 target_type, BOOL allow_pipe)
{
    s16 queue[BOARD_SPACE_MAX];
    u8 distance[BOARD_SPACE_MAX + 1];
    s16 count = BoardSpaceCountGet(0);
    s16 head = 0;
    s16 tail = 0;

    if (start <= 0 || start > count || count > BOARD_SPACE_MAX) {
        return 0;
    }
    memset(distance, 0xFF, sizeof(distance));
    queue[tail++] = start;
    distance[start] = 0;
    while (head < tail) {
        s16 id = queue[head++];
        BoardSpace *space = BoardSpaceGet(0, id);
        if (!allow_pipe && (space->flag & 0x20000000)) {
            continue;
        }
        if (target_space ? id == target_space : space->type == target_type) {
            return distance[id];
        }
        if (distance[id] < 30) {
            for (s16 i = 0; i < space->link_cnt; i++) {
                s16 next = space->link[i];
                if (next > 0 && next <= count && distance[next] == 0xFF &&
                    CheckPathSpace(BoardSpaceGet(0, next))) {
                    distance[next] = distance[id] + 1;
                    queue[tail++] = next;
                }
            }
        }
    }
    return 0;
}

s16 BoardComPathShortcutLenGet(s16 space, u32 type, s32 allow_pipe) {
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        return 0;
    }
    return ShortestPath(space, 0, type, allow_pipe);
}

s16 BoardComPathBestGet(s16 space) {
    PathNode *link_node;
    PathNode *node;
    s32 i;

    PopulateCandidates(space, 30);
    node = SearchPathNodeSpace(space);
    if (!node || node->star != 0) {
        return -1;
    }
    if (node->num_links <= 1) {
        return -1;
    }
    for (i = 0; i < node->num_links; i++) {
        link_node = SearchPathNodeSpace(node->links[i]);
        if (link_node && CanReachTarget(link_node, 0, 1u << NODE_INDEX(node))) {
            return node->links[i];
        }
    }
    return -1;
}

s16 BoardComPathLenGet(s16 space, s16 space_other) {
    return ShortestPath(space, space_other, 0, FALSE);
}

static BOOL PathContainsTarget(PathNode *node, u32 flag) {
    if (!flag) {
        return node->star;
    }
    if (flag == 0x08000000) {
        return node->boo;
    }
    return (node->space_flags & flag) != 0;
}

static BOOL CanReachTarget(PathNode *node, u32 flag, u32 visited) {
    u32 bit = 1u << NODE_INDEX(node);
    if (visited & bit) {
        return FALSE;
    }
    if (PathContainsTarget(node, flag)) {
        return TRUE;
    }
    for (s32 i = 0; i < node->num_links; i++) {
        PathNode *next = SearchPathNodeSpace(node->links[i]);
        if (next && CanReachTarget(next, flag, visited | bit)) {
            return TRUE;
        }
    }
    return FALSE;
}

s16 BoardComPathBestGetFlag(s16 space, u32 flag, s16 len) {
    PathNode *node;
    PathNode *node_link;
    s32 i;

    PopulateCandidates(space, len);
    node = SearchPathNodeSpace(space);
    if (!node || PathContainsTarget(node, flag)) {
        return -1;
    }
    if (node->num_links <= 1) {
        return -1;
    }
    for (i = 0; i < node->num_links; i++) {
        node_link = SearchPathNodeSpace(node->links[i]);
        if (node_link && CanReachTarget(node_link, flag, 1u << NODE_INDEX(node))) {
            return node->links[i];
        }
    }
    return -1;
}
