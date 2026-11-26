/*
  The watch list: the memory addresses the console-side agent reads every
  frame, and the size limits of the snapshot built from it. See
  PROTOCOL.md for the wire format. Both sides keep a matching copy of
  this file.

  Why a list of addresses rather than fixed offsets: achievement
  addresses differ per game, so the client derives the list from the
  game's achievement set (via rcheevos) and sends it to the console once
  per image.

  Why individual values rather than memory ranges: a real set (NFS
  Underground 2) has 482 addresses spread over 192 KB in tight clusters.
  As contiguous ranges that is ~3152 bytes, three packets per frame; as
  individual values about 1350 bytes, two packets.

  Values in the snapshot follow watch-list order, so addresses are not
  sent over the wire: the client generated the list and knows it.
*/

#ifndef __RA_WATCH_H__
#define __RA_WATCH_H__

#define RA_WATCH_MAGIC   0x4C574152
#define RA_WATCH_VERSION 1
#define RA_WATCH_MAX 4096
#define RA_SNAP_HEAD_BYTES 191
#define RA_SNAP_CHUNK_BYTES (1472 - RA_SNAP_HEAD_BYTES)
#define RA_SNAP_PARTS           9
#define RA_SNAP_PARTS_PER_FRAME 3
#define RA_SNAP_MAX_BYTES (RA_SNAP_CHUNK_BYTES * RA_SNAP_PARTS)
#define RA_WATCH_ADDR(e)          ((e)&0x0FFFFFFF)
#define RA_WATCH_SIZE(e)          ((e) >> 28)
#define RA_WATCH_PACK(addr, size) (((unsigned int)(size) << 28) | ((addr)&0x0FFFFFFF))

struct ra_watch_file
{
    unsigned int magic;
    unsigned int version;
    unsigned int count;
    unsigned int bytes;
};

#define RA_NODE_MAGIC 0x4C4E4152
#define RA_NODE_MAX 128
#define RA_NODE_PAIR_BYTES 8

struct ra_node_file
{
    unsigned int magic;
    unsigned int count;
};

struct ra_node
{
    unsigned int w;
    unsigned int offset;
};

#define RA_NODE_PARENT(w)    ((w)&0x0FFF)
#define RA_NODE_FROM_NODE(w) (((w) >> 12) & 1)
#define RA_NODE_SIZE(w)      (((w) >> 13) & 7)
#define RA_NODE_PACK(parent, from_node, size) \
    (((unsigned int)(size) << 13) | ((unsigned int)(from_node) << 12) | ((parent)&0x0FFF))

#endif /* __RA_WATCH_H__ */
