# FeiQ / IPMSG Protocol Reference

## Core Concepts

- Transport: UDP over IPv4, default port 2425; broadcasts use subnet broadcast addresses.
- Datagram format: `VERSION:PACKET_NO:SENDER_NAME:SENDER_HOST:COMMAND:ADDITIONAL` encoded as UTF-8.
- Packet number: 32-bit unsigned integer; unique per sender and incrementing.
- Command field: lower 8 bits indicate base command, higher bits encode options.

## Base Commands

| Command Name | Code (hex) | Direction | Purpose |
| ------------ | ---------- | --------- | ------- |
| BR_ENTRY | 0x00000001 | Broadcast | Announce presence when coming online. |
| ANS_ENTRY | 0x00000003 | Unicast | Reply with presence details to a BR_ENTRY. |
| BR_EXIT | 0x00000002 | Broadcast | Notify peers that the user is going offline. |
| BR_ABSENCE | 0x00000004 | Broadcast | Announce absence status (e.g., away message). |
| SENDMSG | 0x00000020 | Unicast | Deliver a chat message, optional features via options. |
| RECVMSG | 0x00000021 | Unicast | Acknowledge receipt of a SENDMSG when SENDCHECK option is set. |
| READMSG | 0x00000030 | Unicast | Indicate message was read; FeiQ uses as delivery/read receipt. |
| DELMSG | 0x00000031 | Unicast | Request deletion of queued messages (rarely used). |
| GETINFO | 0x00000040 | Unicast | Request detailed user information (status, signature). |
| SENDINFO | 0x00000041 | Unicast | Response to GETINFO containing requested data. |
| GET_ABSENCE_INFO | 0x00000050 | Unicast | Request predefined absence messages. |
| SEND_ABSENCE_INFO | 0x00000051 | Unicast | Provide absence message content. |

## Option Flags

| Option Name | Code (hex) | Meaning |
| ----------- | ---------- | ------- |
| SENDCHECK | 0x00000100 | Sender expects RECVMSG acknowledgment. |
| SECRETOPT | 0x00000200 | Message should not be displayed automatically. |
| BROADCASTOPT | 0x00000400 | Message is broadcast to all peers. |
| ABSENCEOPT | 0x00000800 | Sender is in absence mode (away). |
| SERVEROPT | 0x00001000 | Indicates a host acting as message proxy. |
| DIALUPOPT | 0x00002000 | Sender is on dial-up; avoid large transfers. |
| FILEATTACHOPT | 0x00200000 | Additional section contains file descriptors (IPMSG_FILEATTACH). |
| ENCRYPTOPT | 0x00400000 | Payload is encrypted (FeiQ rarely uses). |

## Additional Field Encoding

The `ADDITIONAL` segment uses `\a` (bell) as a delimiter inside the string.

- Presence (BR_ENTRY/ANS_ENTRY): `username\ahost\aversion\atimestamp[\aext_fields]` in FeiQ.
- Chat messages (SENDMSG): `message_text[\aext_data]`. FeiQ puts plain text first, then optional metadata.
- File attachments: appended after message, encoded per `IPMSG_FILELIST`. Each entry: `file_id:file_name:file_size:mtime:file_attrs` separated by `\a`.

FeiQ-specific extensions include:

- UTF-8 content by default; older IPMSG used Shift-JIS/GBK.
- Extended user profile: `nickname`, `signature`, `mac_address`, `avatar_hash` embedded in ext_fields.
- Group chat: FeiQ encodes group identifier in ext_fields with key-value pairs like `groupid=...`.

## Behaviour Expectations

1. **Presence Handling**
   - On startup: send `BR_ENTRY` to all broadcast addresses.
   - On receipt of `BR_ENTRY`: respond with `ANS_ENTRY` unicast; update or insert user record.
   - Periodic keepalive: resend `BR_ENTRY` every ~60 seconds (FeiQ default). Send `BR_EXIT` before shutdown.

2. **Messaging**
   - `SENDMSG` includes options for acknowledgments (`SENDCHECK`) and attachments (`FILEATTACHOPT`).
   - Receivers must send `RECVMSG` when `SENDCHECK` set, echoing original packet number in `ADDITIONAL`.
   - `READMSG` optionally sent after displaying to user; FeiQ uses it for delivery receipts.

3. **Absence / Status**
   - Absence mode advertised via `ABSENCEOPT` flag and optional absence message appended.
   - Detailed status queries use `GETINFO`/`SENDINFO` round trip.

4. **Files**
   - File transfer is negotiated via `SENDMSG` + `FILEATTACHOPT`; FeiQ then opens a separate TCP port (default 2425 or negotiated) for the actual transfer protocol.
   - Attachment descriptors inform peer about name, size, modify time, permissions.

## Current Mapping in Kylin Messenger

| Feature | Implementation |
| ------- | -------------- |
| Presence broadcast | `BR_ENTRY`/`ANS_ENTRY`/`BR_EXIT`（仅保留 IPMSG，已移除自定义头部与序列化） |
| Reliable messaging | UDP `SENDMSG` + `SENDCHECK`，接收方回 `RECVMSG`；发送端按超时重试，重试上限后报错 |
| Read receipt | 使用 `READMSG`，打开消息后由本地发送，接收方更新已读状态 |
| Group/broadcast | 通过 `BROADCASTOPT`，并在 `additional` 中携带 `GROUP:group_id:content` 约定 |
| File share offer | `FILEATTACHOPT` 描述符 + 独立 TCP 传输，校验大小/偏移/错误，回 `RELEASEFILES` |
| Typing indicators | 暂不内置（非 IPMSG 标准）；可通过扩展键值保留向下兼容空间 |
| ProtoBuf support | 已移除 |

## Notes

- 本项目已统一到 IPMSG/FeiQ 文本协议，移除了自定义二进制头与 ProtoBuf 路径。
- 可靠消息通过 IPMSG 语义（`SENDCHECK`/`RECVMSG`）实现；仅文件数据使用 TCP 通道。
