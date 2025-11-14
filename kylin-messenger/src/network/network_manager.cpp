#include "network_manager.h"

#include "core/models.h"
#include "network/constants.h"
#include "network/ipmsg.h"
#include "network/payload_tags.h"
#include "qt_compat.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QLoggingCategory>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <algorithm>
#include <limits>

namespace KylinMessenger {

namespace {
Q_LOGGING_CATEGORY(lcIpmsg, "kylin.ipmsg")
[[maybe_unused]] const bool loggingConfigured = []() {
    if (!qEnvironmentVariableIsSet("QT_LOGGING_RULES") || qEnvironmentVariable("QT_LOGGING_RULES").trimmed().isEmpty()) {
        QLoggingCategory::setFilterRules(QStringLiteral("kylin.ipmsg.info=true\nkylin.ipmsg.debug=true"));
    }
    return true;
}();
constexpr QChar kIpMsgDelimiter(0x07);
constexpr QChar kNullChar(0x00);

QString joinAdditional(const QStringList& parts)
{
    return parts.isEmpty() ? QString() : parts.join(kIpMsgDelimiter);
}

QString detectFileContentType(const QString& filename)
{
    const QString ext = QFileInfo(filename).suffix().toLower();
    if (ext == QStringLiteral("png")) {
        return QStringLiteral("image/png");
    }
    if (ext == QStringLiteral("jpg") || ext == QStringLiteral("jpeg")) {
        return QStringLiteral("image/jpeg");
    }
    if (ext == QStringLiteral("gif")) {
        return QStringLiteral("image/gif");
    }
    if (ext == QStringLiteral("bmp")) {
        return QStringLiteral("image/bmp");
    }
    if (ext == QStringLiteral("webp")) {
        return QStringLiteral("image/webp");
    }
    return QString();
}

QStringList splitAdditional(const QString& additional)
{
    if (additional.isEmpty()) {
        return {};
    }
    return additional.split(kIpMsgDelimiter, KYLIN_SPLIT_KEEP_EMPTY);
}

QStringList splitFeiQFields(const QString& additional)
{
    if (additional.isEmpty() || !additional.contains(kNullChar)) {
        return {};
    }

    QStringList rawFields = additional.split(kNullChar, KYLIN_SPLIT_KEEP_EMPTY);

    // Remove trailing empty placeholders for cleaner presentation
    while (!rawFields.isEmpty() && rawFields.constLast().isEmpty()) {
        rawFields.removeLast();
    }
    return rawFields;
}

QMap<QString, QString> buildFeiQDetailMap(const QStringList& fields)
{
    QMap<QString, QString> details;
    const auto assignIfPresent = [&](const QString& key, int index) {
        if (index >= 0 && index < fields.size()) {
            const QString value = fields.at(index).trimmed();
            if (!value.isEmpty()) {
                details.insert(key, value);
            }
        }
    };

    assignIfPresent(QStringLiteral("nickname"), 0);
    assignIfPresent(QStringLiteral("group"), 1);
    assignIfPresent(QStringLiteral("mac_address"), 2);
    assignIfPresent(QStringLiteral("login_name"), 3);
    assignIfPresent(QStringLiteral("department"), 4);
    assignIfPresent(QStringLiteral("position"), 5);
    assignIfPresent(QStringLiteral("phone"), 6);
    assignIfPresent(QStringLiteral("mobile"), 7);
    assignIfPresent(QStringLiteral("ip_address"), 8);
    assignIfPresent(QStringLiteral("user_level"), 9);
    assignIfPresent(QStringLiteral("capabilities"), 10);
    assignIfPresent(QStringLiteral("signature"), 11);
    assignIfPresent(QStringLiteral("host_name"), 12);
    assignIfPresent(QStringLiteral("client_type"), 13);
    assignIfPresent(QStringLiteral("client_version"), 14);

    // Preserve the full raw payload for troubleshooting if anything unusual appears.
    if (!fields.isEmpty()) {
        QString rawJoined = fields.join(QStringLiteral(" | "));
        constexpr qsizetype kMaxRawLength = 2048;
        if (rawJoined.size() > kMaxRawLength) {
            rawJoined.truncate(kMaxRawLength);
            rawJoined.append(QStringLiteral(" …"));
        }
        details.insert(QStringLiteral("raw_fields"), rawJoined);
    }

    return details;
}

QString buildFeiQAdditional(const UserInfo& user,
                            const QMap<QString, QString>& details)
{
    QStringList fields;
    fields.reserve(15);
    for (int i = 0; i < 15; ++i) {
        fields.append(QString());
    }

    const auto valueFor = [&](const QString& key) -> QString {
        return details.value(key).trimmed();
    };

    fields[0] = valueFor(QStringLiteral("nickname"));
    if (fields[0].isEmpty()) {
        fields[0] = user.username;
    }

    fields[1] = valueFor(QStringLiteral("group"));
    if (fields[1].isEmpty()) {
        fields[1] = user.group_name;
    }

    fields[2] = valueFor(QStringLiteral("mac_address"));
    fields[3] = valueFor(QStringLiteral("login_name"));
    fields[4] = valueFor(QStringLiteral("department"));
    fields[5] = valueFor(QStringLiteral("position"));
    fields[6] = valueFor(QStringLiteral("phone"));
    fields[7] = valueFor(QStringLiteral("mobile"));

    fields[8] = valueFor(QStringLiteral("ip_address"));
    if (fields[8].isEmpty()) {
        fields[8] = user.ip_address;
    }

    fields[9] = valueFor(QStringLiteral("user_level"));
    fields[10] = valueFor(QStringLiteral("capabilities"));

    fields[11] = valueFor(QStringLiteral("signature"));
    if (fields[11].isEmpty()) {
        fields[11] = user.status_text;
    }

    fields[12] = valueFor(QStringLiteral("host_name"));
    if (fields[12].isEmpty()) {
        fields[12] = user.hostname;
    }

    fields[13] = valueFor(QStringLiteral("client_type"));
    fields[14] = valueFor(QStringLiteral("client_version"));

    return fields.join(kNullChar);
}
} // namespace

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent),
      m_udp_socket(new QUdpSocket(this)),
      m_presence_timer(new QTimer(this)),
      m_cleanup_timer(new QTimer(this)),
      m_retry_timer(new QTimer(this)),
      m_file_server(new QTcpServer(this))
{
    m_presence_timer->setInterval(PRESENCE_INTERVAL);
    m_cleanup_timer->setInterval(CLEANUP_INTERVAL);
    m_retry_timer->setInterval(MESSAGE_ACK_TIMEOUT_MS);
    m_packet_seed = QRandomGenerator::global()->generate();
    if (m_packet_seed == 0) {
        m_packet_seed = 1;
    }
    m_file_id_seed = QRandomGenerator::global()->generate();
    if (m_file_id_seed == 0) {
        m_file_id_seed = 1;
    }
    setupConnections();
}

NetworkManager::~NetworkManager()
{
    shutdown();
}

bool NetworkManager::initialize(const UserInfo& user_info)
{
    m_local_user = user_info;

    if (m_local_user.username.trimmed().isEmpty()) {
        m_local_user.username = QHostInfo::localHostName();
    }
    if (m_local_user.hostname.trimmed().isEmpty()) {
    m_local_user.hostname = QHostInfo::localHostName();
    }
    if (m_local_user.ip_address.trimmed().isEmpty()) {
    m_local_user.ip_address = getLocalIPAddress();
    }
    refreshLocalAddresses();
    m_local_user.port = Network::kUdpDiscoveryPort;
    m_local_user.status = UserStatus::Online;
    if (m_local_user.user_id.isEmpty()) {
        m_local_user.user_id = makeUserId(m_local_user.username,
                                          QHostAddress(m_local_user.ip_address));
    }

    qCInfo(lcIpmsg) << "Initializing IPMSG" << "user" << m_local_user.username
                    << "host" << m_local_user.hostname
                    << "ip" << m_local_user.ip_address
                    << "id" << m_local_user.user_id;

    if (!m_udp_socket->bind(QHostAddress::AnyIPv4,
                            Network::kUdpDiscoveryPort,
                            QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit networkError(QCoreApplication::translate("NetworkManager", "Failed to bind UDP socket: %1").arg(m_udp_socket->errorString()));
        return false;
    }
    
    m_broadcast_addresses = getBroadcastAddresses();
    qCInfo(lcIpmsg) << "Local broadcast addresses" << m_broadcast_addresses;
    broadcastPresence();
    m_presence_timer->start();
    m_cleanup_timer->start();
    m_retry_timer->start();
    setupFileServer();
    
    return true;
}

void NetworkManager::shutdown()
{
    qCInfo(lcIpmsg) << "Shutting down IPMSG manager" << m_local_user.user_id;
    if (m_presence_timer) {
    m_presence_timer->stop();
    }
    if (m_cleanup_timer) {
    m_cleanup_timer->stop();
    }
    if (m_retry_timer) {
        m_retry_timer->stop();
    }

    if (m_udp_socket) {
        m_udp_socket->close();
    }

    if (m_file_server) {
        m_file_server->close();
    }

    const auto closeSockets = [](auto& map) {
        for (QTcpSocket* socket : map.keys()) {
            if (socket) {
        socket->disconnectFromHost();
                if (socket->state() != QAbstractSocket::UnconnectedState) {
                    socket->close();
                }
        socket->deleteLater();
    }
        }
        map.clear();
    };

    closeSockets(m_active_send_transfers);
    closeSockets(m_active_receive_transfers);
    m_pending_send_transfers.clear();
    m_shared_files.clear();
    m_incoming_file_offers.clear();

    m_online_users.clear();
    m_user_last_seen.clear();
    m_pending_messages.clear();
    m_local_addresses.clear();
    m_user_details.clear();
    m_loopback_peer_ids.clear();
    m_loopback_addresses.clear();
    m_loopback_address_map.clear();
}

UserInfo NetworkManager::getLocalUser() const
{
    return m_local_user;
}

QList<UserInfo> NetworkManager::getOnlineUsers() const
{
    return m_online_users.values();
}

void NetworkManager::updateLocalFeiqDetails(const QMap<QString, QString>& details)
{
    m_local_feiq_details = details;
    if (m_udp_socket && m_udp_socket->isOpen()) {
        broadcastPresence();
    }
}

QMap<QString, QString> NetworkManager::getUserDetails(const QString& userId) const
{
    return m_user_details.value(userId);
}

bool NetworkManager::acceptFile(const QString& sender_id,
                                quint32 packet_no,
                                quint32 file_id,
                                const QString& save_path)
{
    const quint64 key = makeFileKey(packet_no, file_id);
    if (!m_incoming_file_index.contains(key)) {
        qCWarning(lcIpmsg) << "Accept file failed; offer not found"
                           << "packet" << packet_no
                           << "file" << file_id
                           << "requestedBy" << sender_id;
        emit networkError(QCoreApplication::translate("NetworkManager", "Requested file offer not found (packet %1, file %2)")
                              .arg(packet_no)
                              .arg(file_id));
        return false;
    }

    IncomingFileOffer offer = m_incoming_file_index.value(key);
    if (!sender_id.isEmpty() && !offer.sender_id.isEmpty() && offer.sender_id != sender_id) {
        qCWarning(lcIpmsg) << "Accept file failed; sender mismatch"
                           << "expected" << offer.sender_id
                           << "actual" << sender_id;
        emit networkError(QCoreApplication::translate("NetworkManager", "File offer belongs to another sender"));
        return false;
    }

    const quint16 port = Network::kUdpDiscoveryPort;  /* temporary fix: use discovery port since sender_port is not found */

    qCInfo(lcIpmsg) << "ACCEPT FILE"
                    << "packet" << packet_no
                    << "file" << file_id
                    << "from" << offer.sender_id
                    << "address" << offer.sender_address.toString()
                    << "port" << port
                    << "save" << QDir::toNativeSeparators(save_path);

    QFileInfo targetInfo(save_path);
    QDir dir = targetInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(QStringLiteral("."))) {
            emit networkError(QCoreApplication::translate("NetworkManager", "Cannot create directory for %1").arg(save_path));
            return false;
        }
    }

    auto socket = new QTcpSocket(this);
    socket->setProxy(QNetworkProxy::NoProxy);
    auto handle = QSharedPointer<QFile>::create(targetInfo.absoluteFilePath());
    if (!handle->open(QIODevice::WriteOnly)) {
        emit networkError(QCoreApplication::translate("NetworkManager", "Cannot open %1 for writing: %2")
                              .arg(save_path, handle->errorString()));
        socket->deleteLater();
        return false;
    }

    ActiveReceiveTransfer transfer;
    transfer.offer = offer;
    transfer.save_path = targetInfo.absoluteFilePath();
    transfer.expected_size = offer.file_size;
    transfer.handle = std::move(handle);

    m_active_receive_transfers.insert(socket, std::move(transfer));

    connect(socket, &QTcpSocket::readyRead,
            this, &NetworkManager::handleReceiveSocketReadyRead);
    connect(socket, &QTcpSocket::disconnected,
            this, &NetworkManager::handleReceiveSocketDisconnected);
    connect(socket,
            KYLIN_QTCP_ERROR_SIGNAL,
            this,
            &NetworkManager::handleReceiveSocketError);

    qCDebug(lcIpmsg) << "Request GETFILEDATA"
                     << "packet" << packet_no
                     << "file" << file_id
                     << "target" << offer.sender_address.toString();

    if (!sendGetFileData(offer.sender_address, packet_no, file_id, 0)) {
        qCWarning(lcIpmsg) << "Failed to send GETFILEDATA command"
                           << "packet" << packet_no
                           << "file" << file_id;
        finalizeReceiveTransfer(socket, false, QStringLiteral("Failed to send GETFILEDATA command"));
        return false;
    }

    connect(socket, &QTcpSocket::connected, this, [socket]() {
        qCInfo(lcIpmsg) << "Receive socket connected"
                        << socket->peerAddress().toString()
                        << "port" << socket->peerPort();
    });

    // IPMSG 文件传输默认让接收方主动连接发送方监听端口
    qCDebug(lcIpmsg) << "Connecting to sender"
                     << offer.sender_address.toString()
                     << "port" << port;
    socket->connectToHost(offer.sender_address, port);

    return true;
}

bool NetworkManager::sendMessage(const QString& receiver_id, const ChatMessage& message)
{
    const auto it = m_online_users.constFind(receiver_id);
    if (it == m_online_users.cend()) {
        emit networkError(QCoreApplication::translate("NetworkManager", "Receiver %1 not found").arg(receiver_id));
        return false;
    }

    QString payload = message.content;
    if (message.message_type == MessageContentType::Image &&
        !Network::PayloadTags::hasImagePrefix(payload)) {
        payload = Network::PayloadTags::applyImagePrefix(payload);
    }

    Network::IPMSG::Packet packet;
    packet.packet_no = nextPacketNo();
    packet.sender_name = m_local_user.username;
    packet.sender_host = m_local_user.hostname;
    packet.command = Network::IPMSG::SENDMSG | Network::IPMSG::SENDCHECK;
    packet.additional = joinAdditional(QStringList{payload});

    const QHostAddress target(it->ip_address);
    const quint16 port = it->port == 0 ? Network::kUdpDiscoveryPort : it->port;

    qCInfo(lcIpmsg) << "SENDMSG" << packet.packet_no << "to" << target.toString()
                    << "port" << port << "len" << message.content.size();
    if (!sendIpMsg(target, port, packet)) {
        return false;
    }

    PendingMessage pending;
    pending.message = message;
    pending.message.content = payload;
    if (pending.message.message_id.isEmpty()) {
        pending.message.message_id = QString::number(packet.packet_no);
    }
    pending.target_address = target;
    pending.target_port = port;
    pending.timestamp = QDateTime::currentDateTime();
    pending.retries = 0;
    pending.packet = packet;

    m_pending_messages.insert(packet.packet_no, pending);
    qCDebug(lcIpmsg) << "Pending ACK for packet" << packet.packet_no;
    return true;
}

bool NetworkManager::sendGroupMessage(const QString& group_id, const ChatMessage& message)
{
    if (m_broadcast_addresses.isEmpty()) {
        m_broadcast_addresses = getBroadcastAddresses();
    }

    QString payload = message.content;
    if (message.message_type == MessageContentType::Image &&
        !Network::PayloadTags::hasImagePrefix(payload)) {
        payload = Network::PayloadTags::applyImagePrefix(payload);
    }

    // 在消息内容前添加群组ID标识，格式：GROUP:group_id:content
    QString group_payload = QStringLiteral("GROUP:%1:%2").arg(group_id, payload);

    Network::IPMSG::Packet packet;
    packet.packet_no = nextPacketNo();
    packet.sender_name = m_local_user.username;
    packet.sender_host = m_local_user.hostname;
    packet.command = Network::IPMSG::SENDMSG | Network::IPMSG::BROADCASTOPT | Network::IPMSG::SENDCHECK;
    packet.additional = joinAdditional(QStringList{group_payload});

    qCInfo(lcIpmsg) << "Group SENDMSG" << packet.packet_no << "group" << group_id 
                    << "targets" << m_broadcast_addresses.size();
    bool sent_any = false;
    for (const QHostAddress& address : m_broadcast_addresses) {
        sent_any |= sendIpMsg(address, Network::kUdpDiscoveryPort, packet);
    }
    return sent_any;
}

bool NetworkManager::sendBroadcastMessage(const ChatMessage& message)
{
    if (m_broadcast_addresses.isEmpty()) {
        m_broadcast_addresses = getBroadcastAddresses();
    }

    QString payload = message.content;
    if (message.message_type == MessageContentType::Image &&
        !Network::PayloadTags::hasImagePrefix(payload)) {
        payload = Network::PayloadTags::applyImagePrefix(payload);
    }

    Network::IPMSG::Packet packet;
    packet.packet_no = nextPacketNo();
    packet.sender_name = m_local_user.username;
    packet.sender_host = m_local_user.hostname;
    packet.command = Network::IPMSG::SENDMSG | Network::IPMSG::BROADCASTOPT;
    packet.additional = joinAdditional(QStringList{payload});

    qCInfo(lcIpmsg) << "Broadcast SENDMSG" << packet.packet_no << "targets" << m_broadcast_addresses.size();
    bool sent_any = false;
    for (const QHostAddress& address : m_broadcast_addresses) {
        sent_any |= sendIpMsg(address, Network::kUdpDiscoveryPort, packet);
    }
    return sent_any;
}

bool NetworkManager::sendFile(const QString& receiver_id,
                              const QString& filepath,
                              quint32* out_packet_no,
                              quint32* out_file_id)
{
    if (out_packet_no) {
        *out_packet_no = 0;
    }
    if (out_file_id) {
        *out_file_id = 0;
    }

    const auto it = m_online_users.constFind(receiver_id);
    if (it == m_online_users.cend()) {
        emit networkError(QCoreApplication::translate("NetworkManager", "Receiver %1 not found").arg(receiver_id));
        return false;
    }

    QFileInfo fileInfo(filepath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit networkError(QCoreApplication::translate("NetworkManager", "File %1 does not exist or is not a regular file").arg(filepath));
        return false;
    }

    if (fileInfo.size() <= 0) {
        qCWarning(lcIpmsg) << "Attempting to send empty file" << filepath;
    }

    const QHostAddress target(it->ip_address);
    const quint16 port = it->port == 0 ? Network::kUdpDiscoveryPort : it->port;

    Network::IPMSG::Packet packet;
    packet.packet_no = nextPacketNo();
    packet.sender_name = m_local_user.username;
    packet.sender_host = m_local_user.hostname;
    packet.command = Network::IPMSG::SENDMSG |
                     Network::IPMSG::SENDCHECK |
                     Network::IPMSG::FILEATTACHOPT;

    const quint32 fileId = nextFileId();
    const quint64 fileSize = static_cast<quint64>(fileInfo.size());
    const quint32 fileMTime = static_cast<quint32>(fileInfo.lastModified().toSecsSinceEpoch());
    const quint32 fileAttr = 0x00000000; // regular file

    const QString messagePart = joinAdditional(QStringList{fileInfo.fileName()});
    const QString attachment = QStringLiteral("%1:%2:%3:%4:%5:%6")
                                   .arg(fileId)
                                   .arg(fileInfo.fileName())
                                   .arg(QString::number(fileSize, 16))
                                   .arg(QString::number(fileMTime, 16))
                                   .arg(QStringLiteral("0"))
                                   .arg(QStringLiteral("0"));

    QString additional = messagePart;
    additional.append(kNullChar);
    additional.append(attachment);
    packet.additional = additional;

    qCInfo(lcIpmsg) << "SEND FILE" << packet.packet_no << "fileId" << fileId
                    << "to" << target.toString() << "size" << fileSize;

    if (!sendIpMsg(target, port, packet)) {
        return false;
    }

    ChatMessage message;
    message.message_id = QString::number(packet.packet_no);
    message.sender_id = m_local_user.user_id;
    message.receiver_id = receiver_id;
    message.message_type = MessageContentType::File;
    message.content = fileInfo.fileName();
    message.timestamp = QDateTime::currentDateTime();

    PendingMessage pending;
    pending.message = message;
    pending.target_address = target;
    pending.target_port = port;
    pending.timestamp = QDateTime::currentDateTime();
    pending.retries = 0;
    pending.packet = packet;
    m_pending_messages.insert(packet.packet_no, pending);

    scheduleFileSend(receiver_id,
                     target,
                     packet.packet_no,
                     fileId,
                     fileInfo.absoluteFilePath(),
                     fileSize,
                     fileAttr,
                     fileMTime);

    if (out_packet_no) {
        *out_packet_no = packet.packet_no;
    }
    if (out_file_id) {
        *out_file_id = fileId;
    }

    return true;
}

bool NetworkManager::sendTypingIndicator(const QString& receiver_id, bool is_typing)
{
    Q_UNUSED(receiver_id);
    Q_UNUSED(is_typing);
        return false;
    }
    
bool NetworkManager::sendReadReceipt(const QString& receiver_id, const QString& message_id)
{
    const auto it = m_online_users.constFind(receiver_id);
    if (it == m_online_users.cend()) {
        emit networkError(QCoreApplication::translate("NetworkManager", "Receiver %1 not found").arg(receiver_id));
        return false;
    }

    bool ok = false;
    quint32 packet_no = message_id.toUInt(&ok);
    if (!ok) {
        packet_no = nextPacketNo();
    }

    Network::IPMSG::Packet packet;
    packet.packet_no = packet_no;
    packet.sender_name = m_local_user.username;
    packet.sender_host = m_local_user.hostname;
    packet.command = Network::IPMSG::READMSG;
    packet.additional = message_id;

    const QHostAddress target(it->ip_address);
    const quint16 port = it->port == 0 ? Network::kUdpDiscoveryPort : it->port;
    qCDebug(lcIpmsg) << "READMSG" << packet.packet_no << "for" << message_id
                     << "target" << target.toString();
    return sendIpMsg(target, port, packet);
}

void NetworkManager::updateLocalUserStatus(UserStatus status, const QString& status_text)
{
    m_local_user.status = status;
    m_local_user.status_text = status_text;
    broadcastPresence();
}

void NetworkManager::updateLocalUserStatus(UserStatus status, const std::string& status_text)
{
    updateLocalUserStatus(status, QString::fromStdString(status_text));
}

void NetworkManager::handleUdpData()
{
    while (m_udp_socket && m_udp_socket->hasPendingDatagrams()) {
        QByteArray buffer;
        buffer.resize(int(m_udp_socket->pendingDatagramSize()));
        
        QHostAddress sender;
        quint16 sender_port = 0;
        const qint64 read = m_udp_socket->readDatagram(buffer.data(),
                                                       buffer.size(),
                                                       &sender,
                                                       &sender_port);
        if (read <= 0) {
            continue;
        }
        
        Network::IPMSG::Packet packet;
        if (!Network::IPMSG::parseDatagram(buffer, packet)) {
            qCWarning(lcIpmsg) << "Failed to parse datagram from" << sender.toString()
                               << "raw" << QString::fromUtf8(buffer);
            continue;
        }
        
        if (isLocalPacket(packet, sender)) {
            qCDebug(lcIpmsg) << "Ignore local packet" << packet.packet_no
                             << "command" << QString::number(packet.command, 16);
                continue;
            }

        qCInfo(lcIpmsg) << "Recv packet" << packet.packet_no
                         << "cmd" << QString::number(packet.command, 16)
                         << "from" << sender.toString() << "port" << sender_port;
        handleIpMsgPacket(packet, sender, sender_port);
    }
}

void NetworkManager::broadcastPresence()
{
    m_broadcast_addresses = getBroadcastAddresses();

    const QString additional = buildLocalPresenceAdditional();

    qCInfo(lcIpmsg) << "Broadcast presence" << m_local_user.username;
    sendBroadcastCommand(Network::IPMSG::BR_ENTRY, additional);
}

void NetworkManager::cleanupOfflineUsers()
{
    const QDateTime now = QDateTime::currentDateTime();
    QList<QString> remove_list;

    for (auto it = m_user_last_seen.cbegin(); it != m_user_last_seen.cend(); ++it) {
        if (it.key() == m_local_user.user_id) {
            continue;
        }

        if (m_loopback_peer_ids.contains(it.key())) {
            continue;
        }

        if (it.value().msecsTo(now) > OFFLINE_TIMEOUT) {
            remove_list.append(it.key());
        }
    }

    for (const QString& user_id : remove_list) {
        qCInfo(lcIpmsg) << "User timeout" << user_id;
        removeUser(user_id);
    }
}

void NetworkManager::retryPendingMessages()
{
    const QDateTime now = QDateTime::currentDateTime();
    QList<quint32> to_remove;

    for (auto it = m_pending_messages.begin(); it != m_pending_messages.end(); ++it) {
        PendingMessage& pending = it.value();
        if (pending.timestamp.msecsTo(now) < MESSAGE_ACK_TIMEOUT_MS) {
            continue;
        }

        if (pending.retries >= MESSAGE_MAX_RETRY) {
            emit networkError(
                QCoreApplication::translate("NetworkManager", "Message %1 not acknowledged after %2 retries").arg(
                    pending.message.message_id, QString::number(MESSAGE_MAX_RETRY)));
            to_remove.append(it.key());
            continue;
        }

        pending.retries += 1;
        pending.timestamp = now;
        qCInfo(lcIpmsg) << "Retry packet" << it.key() << "attempt" << pending.retries;
        sendIpMsg(pending.target_address, pending.target_port, pending.packet);
    }

    for (quint32 key : to_remove) {
        m_pending_messages.remove(key);
    }
}

void NetworkManager::setupConnections()
{
    connect(m_udp_socket,
            &QUdpSocket::readyRead,
            this,
            &NetworkManager::handleUdpData);
    connect(m_presence_timer,
            &QTimer::timeout,
            this,
            &NetworkManager::broadcastPresence);
    connect(m_cleanup_timer,
            &QTimer::timeout,
            this,
            &NetworkManager::cleanupOfflineUsers);
    connect(m_retry_timer,
            &QTimer::timeout,
            this,
            &NetworkManager::retryPendingMessages);
    connect(m_file_server,
            &QTcpServer::newConnection,
            this,
            &NetworkManager::handleFileServerConnection);
}

bool NetworkManager::sendIpMsg(const QHostAddress& target,
                               quint16 port,
                               const Network::IPMSG::Packet& packet)
{
    if (!m_udp_socket) {
        emit networkError(QCoreApplication::translate("NetworkManager", "UDP socket unavailable"));
        return false;
    }
    
    const QByteArray datagram = Network::IPMSG::buildDatagram(packet).toUtf8();
    qCDebug(lcIpmsg) << "Send packet" << packet.packet_no
                     << "cmd" << QString::number(packet.command, 16)
                     << "size" << datagram.size()
                     << "to" << target.toString() << ":" << port;
    const qint64 written = m_udp_socket->writeDatagram(datagram, target, port);
    if (written == -1) {
        emit networkError(QCoreApplication::translate("NetworkManager", "Failed to send datagram: %1").arg(m_udp_socket->errorString()));
        return false;
    }

        return true;
    }

void NetworkManager::sendBroadcastCommand(Network::IPMSG::Command command,
                                          const QString& additional)
{
    Network::IPMSG::Packet packet;
    packet.packet_no = nextPacketNo();
    packet.sender_name = m_local_user.username;
    packet.sender_host = m_local_user.hostname;
    packet.command = command | Network::IPMSG::BROADCASTOPT;
    packet.additional = additional;

    qCInfo(lcIpmsg) << "Broadcast command" << QString::number(command, 16)
                    << "packet" << packet.packet_no;
    for (const QHostAddress& address : m_broadcast_addresses) {
        sendIpMsg(address, Network::kUdpDiscoveryPort, packet);
    }
}

void NetworkManager::handleIpMsgPacket(const Network::IPMSG::Packet& packet,
                                       const QHostAddress& sender,
                                       quint16 sender_port)
{
    qCDebug(lcIpmsg) << "Dispatch packet" << packet.packet_no
                      << "pure" << QString::number(packet.pureCommand(), 16)
                      << "from" << sender.toString() << "port" << sender_port;
    switch (packet.pureCommand()) {
    case Network::IPMSG::BR_ENTRY:
        handlePresencePacket(packet, sender, false);
        break;
    case Network::IPMSG::ANS_ENTRY:
        handlePresencePacket(packet, sender, true);
        break;
    case Network::IPMSG::BR_EXIT:
        handleExitPacket(packet, sender);
        break;
    case Network::IPMSG::SENDMSG:
        handleSendMessagePacket(packet, sender, sender_port);
        break;
    case Network::IPMSG::RECVMSG:
    case Network::IPMSG::READMSG:
        handleRecvMessagePacket(packet);
        break;
    case Network::IPMSG::GETFILEDATA:
        handleGetFileDataPacket(packet, sender);
        break;
    case Network::IPMSG::RELEASEFILES:
        handleReleaseFilePacket(packet, sender);
        break;
    default:
        break;
    }
}

void NetworkManager::handlePresencePacket(const Network::IPMSG::Packet& packet,
                                          const QHostAddress& sender,
                                          bool is_answer)
{
    // 忽略本机包，避免自触发
    if (isLocalPacket(packet, sender)) {
        qCDebug(lcIpmsg) << "Ignore local presence packet from" << sender.toString();
        return;
    }

    const QStringList fields = splitAdditional(packet.additional);
    qCDebug(lcIpmsg) << "Presence additional raw" << packet.additional
                     << "fields" << fields;
    QString nickname = !fields.isEmpty() ? fields.at(0) : packet.sender_name;
    QString group = fields.size() > 1 ? fields.at(1) : QString();
    QString status_text = fields.size() > 2 ? fields.at(2) : QString();

    QMap<QString, QString> feiqDetails;
    const QStringList feiqFields = splitFeiQFields(packet.additional);
    if (!feiqFields.isEmpty()) {
        nickname = feiqFields.value(0, nickname);
        group = feiqFields.value(1, group);
        if (status_text.isEmpty()) {
            status_text = feiqFields.value(11, status_text);
        }
        feiqDetails = buildFeiQDetailMap(feiqFields);
        if (status_text.isEmpty()) {
            status_text = feiqDetails.value(QStringLiteral("signature"));
        }
    }

            UserInfo info;
    info.username = nickname.isEmpty() ? packet.sender_name : nickname;
    info.hostname = packet.sender_host;
    info.ip_address = sender.toString();
    info.port = Network::kUdpDiscoveryPort;
            info.status = UserStatus::Online;
    info.status_text = status_text;
    info.group_name = group;

    // 尽量复用同一 user_id：优先按 IP 匹配已有用户，避免昵称变化导致重复上线
    QString user_id;
    const QString sender_ip = sender.toString();
    for (auto it = m_online_users.constBegin(); it != m_online_users.constEnd(); ++it) {
        if (it.value().ip_address == sender_ip) {
            user_id = it.key();
            break;
        }
    }
    if (user_id.isEmpty()) {
        user_id = makeUserId(info.username, sender);
    }

    // maintain group members: remove from any old groups
    for (auto it = m_group_members.begin(); it != m_group_members.end(); ++it) {
        if (it->contains(user_id)) {
            it->remove(user_id);
            // emit update for old group
            const QList<UserInfo> members = getGroupMembers(it.key());
            emit groupMembersUpdated(it.key(), members);
        }
    }
    // add to current group if provided
    if (!group.trimmed().isEmpty()) {
        m_group_members[group].insert(user_id);
        emit groupMembersUpdated(group, getGroupMembers(group));
    }

    upsertUser(user_id, info, sender);

    if (!feiqDetails.isEmpty()) {
        m_user_details.insert(user_id, feiqDetails);
    }

    qCInfo(lcIpmsg) << (is_answer ? "ANS_ENTRY" : "BR_ENTRY")
                    << packet.packet_no << "user" << user_id
                    << "status" << info.status_text;

    if (!is_answer) {
        Network::IPMSG::Packet response;
        response.packet_no = nextPacketNo();
        response.sender_name = m_local_user.username;
        response.sender_host = m_local_user.hostname;
        response.command = Network::IPMSG::ANS_ENTRY;
        response.additional = buildLocalPresenceAdditional();
        sendIpMsg(sender, Network::kUdpDiscoveryPort, response);
    }
}

void NetworkManager::handleSendMessagePacket(const Network::IPMSG::Packet& packet,
                                             const QHostAddress& sender,
                                             quint16 sender_port)
{
    const int nullIndex = packet.additional.indexOf(kNullChar);
    const QString messagePart = nullIndex >= 0
        ? packet.additional.left(nullIndex)
        : packet.additional;
    const QString attachmentPart = nullIndex >= 0
        ? packet.additional.mid(nullIndex + 1)
        : QString();

    const QStringList fields = splitAdditional(messagePart);
    qCDebug(lcIpmsg) << "Message additional raw" << packet.additional
                     << "messageFields" << fields;
    QString text = fields.isEmpty() ? QString() : fields.first();

    UserInfo info;
    info.username = packet.sender_name;
    info.hostname = packet.sender_host;
    info.ip_address = sender.toString();
    info.port = sender_port == 0 ? Network::kUdpDiscoveryPort : sender_port;
    info.status = UserStatus::Online;

    const QString user_id = makeUserId(info.username, sender);
    upsertUser(user_id, info, sender);

    QList<IncomingFileOffer> offers;
    const bool has_attachments = packet.hasOption(Network::IPMSG::Option::FILEATTACHOPT) && !attachmentPart.isEmpty();
    if (has_attachments) {
    offers = handleFileAttachments(user_id, sender, sender_port, packet.packet_no, attachmentPart);
    }

    ChatMessage message;
    message.message_id = QString::number(packet.packet_no);
    message.sender_id = user_id;
    message.receiver_id = m_local_user.user_id;
    message.timestamp = QDateTime::currentDateTime();

    if (!offers.isEmpty()) {
        const IncomingFileOffer& primary = offers.first();
        message.message_type = MessageContentType::File;
        message.content = text.isEmpty() ? primary.file_name : text;
        message.metadata.insert(QStringLiteral("file_name"), primary.file_name);
        message.metadata.insert(QStringLiteral("packet_no"), QString::number(packet.packet_no));
        message.metadata.insert(QStringLiteral("file_id"), QString::number(primary.file_id));
        message.metadata.insert(QStringLiteral("size"), QString::number(primary.file_size));
        message.metadata.insert(QStringLiteral("sender_ip"), sender.toString());
        message.metadata.insert(QStringLiteral("transfer_status"), QStringLiteral("offered"));
        const QString contentType = detectFileContentType(primary.file_name);
        if (!contentType.isEmpty()) {
            message.metadata.insert(QStringLiteral("content_type"), contentType);
        }
        if (!text.isEmpty()) {
            message.metadata.insert(QStringLiteral("caption"), text);
        }
        if (offers.size() > 1) {
            QStringList names;
            for (const auto& offer : offers) {
                names.append(offer.file_name);
            }
            message.metadata.insert(QStringLiteral("attachment_names"), names.join(QLatin1Char('\n')));
            message.metadata.insert(QStringLiteral("attachment_count"), QString::number(offers.size()));
        }
    } else {
        if (Network::PayloadTags::hasImagePrefix(text)) {
            message.message_type = MessageContentType::Image;
            message.content = Network::PayloadTags::stripImagePrefix(text);
        } else {
            message.message_type = MessageContentType::PlainText;
            message.content = text;
        }
    }

    qCInfo(lcIpmsg) << "RECV SENDMSG" << packet.packet_no << "from" << user_id
                    << "len" << text.size();
    
    // 检查是否为广播消息（群组消息）
    if (packet.hasOption(Network::IPMSG::Option::BROADCASTOPT)) {
        // 从消息内容中提取群组ID（如果存在）
        // 格式：group_id:message_content 或直接使用默认群组
        QString group_id;
        QString actual_content = text;
        
        // 尝试解析群组ID（格式：GROUP:group_id:content）
        if (text.startsWith(QStringLiteral("GROUP:"))) {
            const int first_colon = text.indexOf(QLatin1Char(':'), 6);
            if (first_colon > 6) {
                group_id = text.mid(6, first_colon - 6);
                actual_content = text.mid(first_colon + 1);
            }
        }
        
        // 如果没有指定群组ID，使用默认广播群组
        if (group_id.isEmpty()) {
            group_id = QStringLiteral("broadcast");
        }
        
        message.group_id = group_id;
        message.content = actual_content;
        if (message.message_type == MessageContentType::Image && 
            Network::PayloadTags::hasImagePrefix(actual_content)) {
            message.content = Network::PayloadTags::stripImagePrefix(actual_content);
        }
        
        qCInfo(lcIpmsg) << "Broadcast/Group message" << packet.packet_no 
                        << "group" << group_id << "from" << user_id;
        emit groupMessageReceived(group_id, message);
    } else {
        emit messageReceived(message);
    }

    if (packet.hasOption(Network::IPMSG::Option::SENDCHECK)) {
        Network::IPMSG::Packet ack;
        ack.packet_no = nextPacketNo();
        ack.sender_name = m_local_user.username;
        ack.sender_host = m_local_user.hostname;
        ack.command = Network::IPMSG::RECVMSG;
        ack.additional = QString::number(packet.packet_no);
        const quint16 ackPort = sender_port == 0 ? Network::kUdpDiscoveryPort : sender_port;
        qCDebug(lcIpmsg) << "SEND RECVMSG" << ack.packet_no << "acknowledging" << packet.packet_no;
        sendIpMsg(sender, ackPort, ack);
    }
}

void NetworkManager::handleRecvMessagePacket(const Network::IPMSG::Packet& packet)
{
    const QStringList fields = splitAdditional(packet.additional);
    bool ok = false;
    const quint32 original_no = fields.isEmpty() ? packet.packet_no : fields.first().toUInt(&ok);
    const quint32 key = ok ? original_no : packet.packet_no;

    if (!m_pending_messages.contains(key)) {
        return;
    }

    PendingMessage pending = m_pending_messages.take(key);
    qCInfo(lcIpmsg) << ((packet.pureCommand() == Network::IPMSG::READMSG) ? "READMSG" : "RECVMSG")
                    << packet.packet_no << "ack for" << key;
    if (packet.pureCommand() == Network::IPMSG::READMSG) {
        emit messageRead(pending.message.message_id);
    }
}

QList<NetworkManager::IncomingFileOffer> NetworkManager::handleFileAttachments(const QString& senderId,
                                                                              const QHostAddress& senderAddress,
                                                                              quint32 senderPort,  // 改为quint32以匹配头文件
                                                                              quint32 packetNo,
                                                                              const QString& attachmentsPart)
{
    QStringList entries = attachmentsPart.split(kIpMsgDelimiter, KYLIN_SPLIT_KEEP_EMPTY);
    if (entries.isEmpty() && !attachmentsPart.isEmpty()) {
        entries = QStringList{attachmentsPart};
    }

    if (entries.isEmpty()) {
        return {};
    }

    const auto parseNumber = [](const QString& value) -> quint64 {
        bool ok = false;
        quint64 result = value.toULongLong(&ok, 16);
        if (!ok) {
            result = value.toULongLong(&ok, 10);
        }
        return ok ? result : 0;
    };

    QList<IncomingFileOffer>& offers = m_incoming_file_offers[senderId];
    QList<IncomingFileOffer> collected;

    for (const QString& entry : entries) {
        if (entry.trimmed().isEmpty()) {
            continue;
        }
    const QStringList parts = entry.split(QLatin1Char(':'), KYLIN_SPLIT_KEEP_EMPTY);
        if (parts.size() < 2) {
            qCWarning(lcIpmsg) << "Invalid file attachment entry" << entry;
            continue;
        }

        bool ok = false;
        const quint32 fileId = parts.at(0).toUInt(&ok, 10);
        if (!ok) {
            qCWarning(lcIpmsg) << "Invalid file id in attachment" << entry;
            continue;
        }

        IncomingFileOffer offer;
        offer.sender_id = senderId;
        offer.sender_address = senderAddress;
        // offer.sender_port = senderPort == 0 ? Network::kUdpDiscoveryPort : senderPort;  /* temporary fix */
        offer.packet_no = packetNo;
        offer.file_id = fileId;
        offer.file_name = parts.size() > 1 ? parts.at(1) : QString();
        offer.file_size = parts.size() > 2 ? parseNumber(parts.at(2)) : 0;
        offer.file_mtime = parts.size() > 3 ? static_cast<quint32>(parseNumber(parts.at(3))) : 0;
        offer.file_attr = parts.size() > 4 ? static_cast<quint32>(parseNumber(parts.at(4))) : 0;

        const quint64 key = makeFileKey(packetNo, fileId);
        m_incoming_file_index.insert(key, offer);

        auto existingIt = std::find_if(offers.begin(), offers.end(), [&](const IncomingFileOffer& existing) {
            return existing.packet_no == packetNo && existing.file_id == fileId;
        });
        if (existingIt != offers.end()) {
            *existingIt = offer;
        } else {
            offers.append(offer);
        }

        collected.append(offer);

        qCInfo(lcIpmsg) << "File offer" << offer.file_name
                        << "size" << offer.file_size
                        << "from" << senderId
                        << "packet" << packetNo
                        << "fileId" << fileId;

        emit fileOfferReceived(senderId,
                               senderAddress.toString(),
                               packetNo,
                               fileId,
                               offer.file_name,
                               offer.file_size);
    }

    return collected;
}

void NetworkManager::handleExitPacket(const Network::IPMSG::Packet&,
                                      const QHostAddress& sender)
{
    const QString user_id = makeUserId(QString(), sender);
    if (m_online_users.contains(user_id)) {
        qCInfo(lcIpmsg) << "BR_EXIT" << user_id;
        // remove from all groups first
        for (auto it = m_group_members.begin(); it != m_group_members.end(); ++it) {
            if (it->contains(user_id)) {
                it->remove(user_id);
                emit groupMembersUpdated(it.key(), getGroupMembers(it.key()));
            }
        }
        removeUser(user_id);
    }
}

void NetworkManager::handleGetFileDataPacket(const Network::IPMSG::Packet& packet,
                                             const QHostAddress& sender)
{
    const QStringList tokens = packet.additional.split(QLatin1Char(':'), KYLIN_SPLIT_KEEP_EMPTY);
    if (tokens.size() < 2) {
        qCWarning(lcIpmsg) << "Malformed GETFILEDATA" << packet.additional;
        return;
    }
    
    bool okPacket = false;
    const quint32 requestPacket = tokens.at(0).toUInt(&okPacket, 10);
    if (!okPacket) {
        qCWarning(lcIpmsg) << "Invalid packet number in GETFILEDATA" << packet.additional;
        return;
    }

    bool okFile = false;
    const quint32 fileId = tokens.at(1).toUInt(&okFile, 10);
    if (!okFile) {
        qCWarning(lcIpmsg) << "Invalid file id in GETFILEDATA" << packet.additional;
        return;
    }

    quint64 offset = 0;
    if (tokens.size() > 2) {
        bool okOffset = false;
        offset = tokens.at(2).toULongLong(&okOffset, 10);
        if (!okOffset) {
            offset = 0;
        }
    }

    const quint64 key = makeFileKey(requestPacket, fileId);
    if (!m_shared_files.contains(key)) {
        qCWarning(lcIpmsg) << "Unknown file requested" << requestPacket << fileId;
        return;
    }

    PendingSendTransfer pending;
    pending.file = m_shared_files.value(key);
    pending.file.peer_address = sender;
    pending.offset = offset;

    const QString ipKey = sender.toString();
    m_pending_send_transfers[ipKey].append(pending);
    qCInfo(lcIpmsg) << "GETFILEDATA" << "packet" << requestPacket
                    << "file" << fileId << "offset" << offset
                    << "from" << sender.toString();

    setupFileServer();
}

void NetworkManager::handleReleaseFilePacket(const Network::IPMSG::Packet& packet,
                                             const QHostAddress& sender)
{
    Q_UNUSED(sender);

    const QStringList tokens = packet.additional.split(QLatin1Char(':'), KYLIN_SPLIT_KEEP_EMPTY);
    if (tokens.size() < 2) {
        return;
    }
    
    bool okPacket = false;
    const quint32 requestPacket = tokens.at(0).toUInt(&okPacket, 10);
    if (!okPacket) {
        return;
    }

    bool okFile = false;
    const quint32 fileId = tokens.at(1).toUInt(&okFile, 10);
    if (!okFile) {
        return;
    }

    const quint64 key = makeFileKey(requestPacket, fileId);
    if (m_shared_files.contains(key)) {
        qCInfo(lcIpmsg) << "RELEASEFILES" << requestPacket << fileId;
    }
}

void NetworkManager::handleFileServerConnection()
{
    if (!m_file_server) {
        return;
    }

    while (m_file_server->hasPendingConnections()) {
        QTcpSocket* socket = m_file_server->nextPendingConnection();
        if (!socket) {
            continue;
        }
        qCInfo(lcIpmsg) << "File server connection from" << socket->peerAddress().toString()
                        << "port" << socket->peerPort();
        processPendingSend(socket);
    }
}

void NetworkManager::processPendingSend(QTcpSocket* socket)
{
    if (!socket) {
        return;
    }

    const QString ipKey = socket->peerAddress().toString();
    auto listIt = m_pending_send_transfers.find(ipKey);
    // If there's no direct match by peerAddress string, try a relaxed search.
    if (listIt == m_pending_send_transfers.end() || listIt->isEmpty()) {
        // Fallback: the stored pending entry might be keyed by a different local
        // address (e.g. host IP) while the incoming TCP connection shows as
        // loopback (127.0.0.1 ::1) or vice versa. Try to find a candidate by
        // comparing stored peer_address values.
        const QString peerStr = socket->peerAddress().toString();
        bool found = false;
        for (auto it = m_pending_send_transfers.begin(); it != m_pending_send_transfers.end(); ++it) {
            if (it->isEmpty()) continue;
            const PendingSendTransfer& candidate = it->first();
            const QString candAddr = candidate.file.peer_address.toString();
            // Exact match
            if (candAddr == peerStr) {
                listIt = it;
                found = true;
                break;
            }
            // Both loopback addresses (127.0.0.1 ::1)
            QHostAddress candHost(candidate.file.peer_address);
            QHostAddress sockHost(socket->peerAddress());
            if (candHost.isLoopback() && sockHost.isLoopback()) {
                listIt = it;
                found = true;
                break;
            }
        }

        if (!found) {
            qCWarning(lcIpmsg) << "No pending file transfer for" << ipKey << "or fallback candidates";
            socket->close();
            socket->deleteLater();
            return;
        }
    }

    PendingSendTransfer pending = listIt->takeFirst();
    if (listIt->isEmpty()) {
        m_pending_send_transfers.erase(listIt);
    }

    auto fileHandle = QSharedPointer<QFile>::create(pending.file.file_path);
    if (!fileHandle->open(QIODevice::ReadOnly)) {
        const QString error = QCoreApplication::translate(
            "NetworkManager", "Cannot open %1: %2").arg(pending.file.file_path, fileHandle->errorString());
        qCWarning(lcIpmsg) << error;
        emit fileTransferFailed(pending.file.peer_id,
                                pending.file.packet_no,
                                pending.file.file_id,
                                error);
        socket->close();
        socket->deleteLater();
        return;
    }

    if (pending.offset > 0) {
        if (!fileHandle->seek(static_cast<qint64>(pending.offset))) {
            const QString error = QCoreApplication::translate(
                "NetworkManager", "Cannot seek %1 to offset %2")
                                      .arg(pending.file.file_path)
                                      .arg(pending.offset);
            qCWarning(lcIpmsg) << error;
            emit fileTransferFailed(pending.file.peer_id,
                                    pending.file.packet_no,
                                    pending.file.file_id,
                                    error);
            socket->close();
            socket->deleteLater();
            return;
        }
    }

    ActiveSendTransfer transfer;
    transfer.file = pending.file;
    transfer.offset = pending.offset;
    transfer.bytes_sent = pending.offset;
    transfer.handle = std::move(fileHandle);

    m_active_send_transfers.insert(socket, std::move(transfer));

    connect(socket, &QTcpSocket::bytesWritten,
            this, &NetworkManager::handleFileSocketBytesWritten);
    connect(socket, &QTcpSocket::disconnected,
            this, &NetworkManager::handleFileSocketDisconnected);
    connect(socket, KYLIN_QTCP_ERROR_SIGNAL,
            this, &NetworkManager::handleFileSocketError);

    pumpFileSocket(socket);
}

void NetworkManager::pumpFileSocket(QTcpSocket* socket)
{
    if (!socket) {
        return;
    }

    auto it = m_active_send_transfers.find(socket);
    if (it == m_active_send_transfers.end()) {
        return;
    }
    
    ActiveSendTransfer& transfer = it.value();
    if (!transfer.handle || !transfer.handle->isOpen()) {
        finalizeSendTransfer(socket, false, QStringLiteral("File handle unavailable"));
        return;
    }

    const qint64 kChunkSize = 64 * 1024;
    while (socket->state() == QAbstractSocket::ConnectedState &&
           socket->bytesToWrite() < 256 * 1024) {
        if (transfer.bytes_sent >= transfer.file.file_size) {
            socket->disconnectFromHost();
            break;
        }
            
        QByteArray chunk = transfer.handle->read(kChunkSize);
        if (chunk.isEmpty()) {
            socket->disconnectFromHost();
            break;
    }

        const qint64 written = socket->write(chunk);
        if (written < 0) {
            finalizeSendTransfer(socket, false, socket->errorString());
            return;
        }

        transfer.bytes_sent += static_cast<quint64>(written);
        emit fileTransferProgress(transfer.file.peer_id,
                                  transfer.file.packet_no,
                                  transfer.file.file_id,
                                  transfer.bytes_sent,
                                  transfer.file.file_size);
    }
}

void NetworkManager::handleFileSocketBytesWritten(qint64)
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }
    pumpFileSocket(socket);
}

void NetworkManager::handleFileSocketDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    auto it = m_active_send_transfers.find(socket);
    if (it == m_active_send_transfers.end()) {
        socket->deleteLater();
        return;
    }

    const bool success = it.value().bytes_sent >= it.value().file.file_size;
    finalizeSendTransfer(socket, success,
                         success ? QString() : QStringLiteral("Transfer ended prematurely"));
}

void NetworkManager::handleFileSocketError(QAbstractSocket::SocketError)
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }
    finalizeSendTransfer(socket, false, socket->errorString());
}

void NetworkManager::handleReceiveSocketReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    auto it = m_active_receive_transfers.find(socket);
    if (it == m_active_receive_transfers.end()) {
        return;
    }

    ActiveReceiveTransfer& transfer = it.value();
    if (!transfer.handle || !transfer.handle->isOpen()) {
        finalizeReceiveTransfer(socket, false, QStringLiteral("File handle unavailable"));
        return;
    }

    const QByteArray data = socket->readAll();
    if (data.isEmpty()) {
        return;
    }

    const qint64 written = transfer.handle->write(data);
    if (written != data.size()) {
        finalizeReceiveTransfer(socket, false, QStringLiteral("Failed to write file"));
        return;
    }

    transfer.bytes_received += static_cast<quint64>(written);
    emit fileTransferProgress(transfer.offer.sender_id,
                              transfer.offer.packet_no,
                              transfer.offer.file_id,
                              transfer.bytes_received,
                              transfer.offer.file_size);
}

void NetworkManager::handleReceiveSocketDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    auto it = m_active_receive_transfers.find(socket);
    if (it == m_active_receive_transfers.end()) {
        socket->deleteLater();
        return;
    }

    const ActiveReceiveTransfer& transfer = it.value();
    const bool success = (transfer.expected_size == 0) ||
                         (transfer.bytes_received >= transfer.expected_size);
    qCInfo(lcIpmsg) << "Receive socket disconnected"
                    << socket->peerAddress().toString()
                    << "port" << socket->peerPort()
                    << "received" << transfer.bytes_received
                    << "expected" << transfer.expected_size
                    << "success" << success;
    finalizeReceiveTransfer(socket, success,
                            success ? QString() : QStringLiteral("Transfer ended prematurely"));
}

void NetworkManager::handleReceiveSocketError(QAbstractSocket::SocketError error)
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }
    qCWarning(lcIpmsg) << "Receive socket error"
                       << socket->peerAddress().toString()
                       << "port" << socket->peerPort()
                       << "error" << error
                       << socket->errorString();
    finalizeReceiveTransfer(socket, false, socket->errorString());
}

QString NetworkManager::makeUserId(const QString& username,
                                   const QHostAddress& address) const
{
    const QString ip = address.toString();
    if (m_loopback_address_map.contains(ip)) {
        return m_loopback_address_map.value(ip);
    }
    if (!ip.isEmpty() && ip != QLatin1String("0.0.0.0")) {
        return ip;
    }
    if (!username.isEmpty()) {
        return username;
    }
    return QStringLiteral("unknown");
}

void NetworkManager::upsertUser(const QString& user_id,
                                UserInfo user_info,
                                const QHostAddress& address)
{
    user_info.user_id = user_id;
    user_info.ip_address = address.toString();
    if (user_info.hostname.isEmpty()) {
        user_info.hostname = address.toString();
    }
    user_info.port = Network::kUdpDiscoveryPort;
    user_info.status = UserStatus::Online;

    const bool is_new = !m_online_users.contains(user_id);
    if (!is_new) {
        const UserInfo& existing = m_online_users.value(user_id);
        if (user_info.username.isEmpty()) {
            user_info.username = existing.username;
        }
        if (user_info.hostname.isEmpty()) {
            user_info.hostname = existing.hostname;
        }
        if (user_info.group_name.isEmpty()) {
            user_info.group_name = existing.group_name;
        }
        if (user_info.status_text.isEmpty()) {
            user_info.status_text = existing.status_text;
        }
    }
    m_online_users.insert(user_id, user_info);
    m_user_last_seen[user_id] = QDateTime::currentDateTime();

    if (is_new) {
        qCInfo(lcIpmsg) << "User online" << user_id;
        emit userOnline(user_info);
    } else {
        qCDebug(lcIpmsg) << "User update" << user_id;
        emit userInfoUpdated(user_info);
    }
}

void NetworkManager::removeUser(const QString& user_id)
{
    if (!m_online_users.contains(user_id)) {
        return;
    }

    m_online_users.remove(user_id);
    m_user_last_seen.remove(user_id);
    m_user_details.remove(user_id);
    qCInfo(lcIpmsg) << "User offline" << user_id;
    emit userOffline(user_id);
}

void NetworkManager::registerLoopbackPeer(const UserInfo& user_info)
{
    if (user_info.user_id.isEmpty()) {
        qCWarning(lcIpmsg) << "Loopback peer missing user_id, ignore";
        return;
    }

    QString ip = user_info.ip_address;
    if (ip.isEmpty()) {
        ip = getLocalIPAddress();
    }

    QHostAddress address(ip);
    if (address.isNull()) {
        qCWarning(lcIpmsg) << "Loopback peer invalid address" << ip;
        return;
    }

    const QString ip_string = address.toString();
    m_loopback_peer_ids.insert(user_info.user_id);
    m_loopback_addresses.insert(ip_string);
    m_loopback_address_map.insert(ip_string, user_info.user_id);

    const QString loopback_ip = QHostAddress(QHostAddress::LocalHost).toString();
    m_loopback_addresses.insert(loopback_ip);
    m_loopback_address_map.insert(loopback_ip, user_info.user_id);

    qCInfo(lcIpmsg) << "Register loopback peer" << user_info.user_id
                    << "ip" << ip_string;
    upsertUser(user_info.user_id, user_info, address);
}

void NetworkManager::setupFileServer()
{
    if (!m_file_server) {
        return;
    }
    if (m_file_server->isListening()) {
        return;
    }

    // 监听 TCP 端口（与 UDP 发现端口区分）
    if (!m_file_server->listen(QHostAddress::AnyIPv4, Network::kTcpListenPort)) {
        qCWarning(lcIpmsg) << "Failed to start file server:" << m_file_server->errorString();
    } else {
        qCInfo(lcIpmsg) << "File server listening on TCP port" << m_file_server->serverPort();
    }
}

quint32 NetworkManager::nextFileId()
{
    ++m_file_id_seed;
    if (m_file_id_seed == 0) {
        m_file_id_seed = 1;
    }
    return m_file_id_seed;
}

quint64 NetworkManager::makeFileKey(quint32 packetNo, quint32 fileId) const
{
    return (static_cast<quint64>(packetNo) << 32) | static_cast<quint64>(fileId);
}

void NetworkManager::scheduleFileSend(const QString& receiverId,
                                       const QHostAddress& receiverAddr,
                                       quint32 packetNo,
                                       quint32 fileId,
                                       const QString& filepath,
                                       quint64 filesize,
                                       quint32 fileAttr,
                                       quint32 fileMtime)
{
    SharedFileEntry entry;
    entry.peer_id = receiverId;
    entry.peer_address = receiverAddr;
    entry.file_path = filepath;
    entry.file_name = QFileInfo(filepath).fileName();
    entry.packet_no = packetNo;
    entry.file_id = fileId;
    entry.file_size = filesize;
    entry.file_attr = fileAttr;
    entry.file_mtime = fileMtime;

    const quint64 key = makeFileKey(packetNo, fileId);
    m_shared_files.insert(key, entry);
}

bool NetworkManager::sendGetFileData(const QHostAddress& target,
                                     quint32 packetNo,
                                     quint32 fileId,
                                     quint64 offset)
{
    Network::IPMSG::Packet request;
    request.packet_no = nextPacketNo();
    request.sender_name = m_local_user.username;
    request.sender_host = m_local_user.hostname;
    request.command = Network::IPMSG::GETFILEDATA;

    QStringList parts;
    parts << QString::number(packetNo)
          << QString::number(fileId);
    if (offset > 0) {
        parts << QString::number(offset);
    }
    request.additional = parts.join(QLatin1Char(':'));

    return sendIpMsg(target, Network::kUdpDiscoveryPort, request);
}

void NetworkManager::sendReleaseFiles(const QHostAddress& target,
                                      quint32 packetNo,
                                      quint32 fileId)
{
    Network::IPMSG::Packet release;
    release.packet_no = nextPacketNo();
    release.sender_name = m_local_user.username;
    release.sender_host = m_local_user.hostname;
    release.command = Network::IPMSG::RELEASEFILES;
    release.additional = QStringLiteral("%1:%2")
                             .arg(QString::number(packetNo))
                             .arg(QString::number(fileId));
    sendIpMsg(target, Network::kUdpDiscoveryPort, release);
}

void NetworkManager::finalizeSendTransfer(QTcpSocket* socket,
                                          bool success,
                                          const QString& reason)
{
    if (!socket) {
        return;
    }

    auto it = m_active_send_transfers.find(socket);
    if (it != m_active_send_transfers.end()) {
        ActiveSendTransfer transfer = std::move(it.value());
        m_active_send_transfers.erase(it);
        if (transfer.handle) {
            transfer.handle->close();
        }

        if (success) {
            emit fileTransferFinished(transfer.file.peer_id,
                                      transfer.file.packet_no,
                                      transfer.file.file_id,
                                      QString());
        } else {
            emit fileTransferFailed(transfer.file.peer_id,
                                    transfer.file.packet_no,
                                    transfer.file.file_id,
                                    reason.isEmpty() ? QStringLiteral("Unknown error") : reason);
        }
    }

    socket->deleteLater();
}

void NetworkManager::finalizeReceiveTransfer(QTcpSocket* socket,
                                             bool success,
                                             const QString& reason)
{
    if (!socket) {
        return;
    }

    auto it = m_active_receive_transfers.find(socket);
    if (it != m_active_receive_transfers.end()) {
        ActiveReceiveTransfer transfer = std::move(it.value());
        m_active_receive_transfers.erase(it);
        if (transfer.handle) {
            transfer.handle->close();
        }

        if (success) {
            sendReleaseFiles(transfer.offer.sender_address,
                             transfer.offer.packet_no,
                             transfer.offer.file_id);

            emit fileTransferFinished(transfer.offer.sender_id,
                                      transfer.offer.packet_no,
                                      transfer.offer.file_id,
                                      transfer.save_path);

            const quint64 key = makeFileKey(transfer.offer.packet_no, transfer.offer.file_id);
            m_incoming_file_index.remove(key);
            auto listIt = m_incoming_file_offers.find(transfer.offer.sender_id);
            if (listIt != m_incoming_file_offers.end()) {
                auto& list = *listIt;
                for (int i = 0; i < list.size(); ++i) {
                    if (list.at(i).packet_no == transfer.offer.packet_no &&
                        list.at(i).file_id == transfer.offer.file_id) {
                        list.removeAt(i);
                        break;
                    }
                }
                if (list.isEmpty()) {
                    m_incoming_file_offers.erase(listIt);
                }
            }
        } else {
            emit fileTransferFailed(transfer.offer.sender_id,
                                    transfer.offer.packet_no,
                                    transfer.offer.file_id,
                                    reason.isEmpty() ? QStringLiteral("Unknown error") : reason);
        }
    }

    socket->deleteLater();
}

QString NetworkManager::buildLocalPresenceAdditional() const
{
    return buildFeiQAdditional(m_local_user, m_local_feiq_details);
}

QString NetworkManager::getLocalIPAddress() const
{
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            !address.isLoopback()) {
            return address.toString();
        }
    }
    return QStringLiteral("127.0.0.1");
}

QVector<QHostAddress> NetworkManager::getBroadcastAddresses() const
{
    QVector<QHostAddress> broadcasts;
    QSet<QString> seen;

    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        const auto flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp) ||
            flags.testFlag(QNetworkInterface::IsLoopBack) ||
            flags.testFlag(QNetworkInterface::IsPointToPoint)) {
            continue;
        }

        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            const QHostAddress broadcast = entry.broadcast();
            if (broadcast.isNull() ||
                broadcast.protocol() != QAbstractSocket::IPv4Protocol) {
                continue;
            }

            const QString key = broadcast.toString();
            if (!seen.contains(key)) {
                seen.insert(key);
                broadcasts.append(broadcast);
            }
        }
    }

    return broadcasts;
}

quint32 NetworkManager::nextPacketNo()
{
    ++m_packet_seed;
    if (m_packet_seed == 0) {
        m_packet_seed = 1;
    }
    return m_packet_seed;
}

void NetworkManager::refreshLocalAddresses()
{
    m_local_addresses.clear();

    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        if (!iface.flags().testFlag(QNetworkInterface::IsUp)) {
            continue;
        }

        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            const QHostAddress ip = entry.ip();
            if (ip.isNull()) {
                continue;
            }
            if (ip.protocol() != QAbstractSocket::IPv4Protocol) {
                continue;
            }
            m_local_addresses.insert(ip.toString());
        }
    }

    m_local_addresses.insert(QStringLiteral("127.0.0.1"));
    if (!m_local_user.ip_address.isEmpty()) {
        m_local_addresses.insert(m_local_user.ip_address);
    }

    qCDebug(lcIpmsg) << "Local addresses" << m_local_addresses;
}

bool NetworkManager::isLocalPacket(const Network::IPMSG::Packet& packet,
                                   const QHostAddress& sender) const
{
    const QString sender_ip = sender.toString();
    if (m_loopback_addresses.contains(sender_ip)) {
        if (!packet.hasOption(Network::IPMSG::BROADCASTOPT)) {
            qCDebug(lcIpmsg) << "Packet from registered loopback peer" << sender_ip
                             << "treat as remote";
            return false;
        }
    }

    if (packet.sender_name != m_local_user.username) {
        return false;
    }

    if (!m_local_user.hostname.isEmpty() &&
        packet.sender_host.compare(m_local_user.hostname, Qt::CaseInsensitive) != 0) {
        return false;
    }

    if (sender.isLoopback()) {
        qCDebug(lcIpmsg) << "Packet seen from loopback, treat as local";
        return true;
    }

    const bool is_local = m_local_addresses.contains(sender_ip);
    if (is_local) {
        qCDebug(lcIpmsg) << "Packet from local IP" << sender_ip;
    }
    return is_local;
}

QList<UserInfo> NetworkManager::getGroupMembers(const QString& group_id) const
{
    QList<UserInfo> result;
    if (!m_group_members.contains(group_id)) {
        return result;
    }
    const QSet<QString>& ids = m_group_members.value(group_id);
    result.reserve(ids.size());
    for (const QString& id : ids) {
        auto it = m_online_users.find(id);
        if (it != m_online_users.end()) {
            result.append(it.value());
        }
    }
    return result;
}

} // namespace KylinMessenger
