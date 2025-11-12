// chat_window.cpp - 聊天窗口实现
#include "chat_window.h"
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QRegularExpression>
#include <QUuid>
#include <QBuffer>
#include <QDebug>
#include <QCloseEvent>
#include <QEvent>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextBlockFormat>
#include <QStyle>
#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QPoint>
#include <QFrame>
#include <QImage>
#include <QDateTime>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QUrl>
#include <QDesktopServices>
#include <QTabWidget>
#include <QScrollArea>
#include <QWidgetAction>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QStringList>
#include <algorithm>
#include <QLoggingCategory>

#include "core/di/service_locator.h"
#include "network/payload_tags.h"

namespace KylinMessenger {

namespace {

Q_LOGGING_CATEGORY(lcChatWindow, "kylin.chatwindow");

constexpr char kComplianceVerdictKey[] = "compliance.verdict";
constexpr char kComplianceReasonKey[] = "compliance.reason";

QString verdictCodeFromResult(ComplianceVerdict verdict)
{
    switch (verdict) {
    case ComplianceVerdict::Allowed:
        return QStringLiteral("allowed");
    case ComplianceVerdict::NeedsReview:
        return QStringLiteral("needs_review");
    case ComplianceVerdict::Blocked:
        return QStringLiteral("blocked");
    }
    return QStringLiteral("unknown");
}

QString verdictLabelForImage(const QString& code)
{
    if (code == QLatin1String("allowed")) {
        return QStringLiteral("图片审核：通过");
    }
    if (code == QLatin1String("needs_review")) {
        return QStringLiteral("图片审核：需人工复核");
    }
    if (code == QLatin1String("blocked")) {
        return QStringLiteral("图片审核：已拦截");
    }
    return QString();
}

QString verdictColorForImage(const QString& code)
{
    if (code == QLatin1String("allowed")) {
        return QStringLiteral("#2e7d32");
    }
    if (code == QLatin1String("needs_review")) {
        return QStringLiteral("#ef6c00");
    }
    if (code == QLatin1String("blocked")) {
        return QStringLiteral("#c62828");
    }
    return QStringLiteral("#616161");
}

QString verdictNoteForImage(const QString& code, const QString& reason)
{
    QString base = verdictLabelForImage(code);
    if (base.isEmpty()) {
        return base;
    }
    const QString trimmed = reason.trimmed();
    if (!trimmed.isEmpty()) {
        base.append(QStringLiteral("（%1）").arg(trimmed));
    }
    return base;
}

bool looksLikeBase64Image(const QString& text)
{
    if (text.size() < 60) {
        return false;
    }

    int usefulChars = 0;
    for (const QChar ch : text) {
        if (ch.isSpace()) {
            continue;
        }
        const ushort u = ch.unicode();
        const bool isBase64Char = (u >= 'A' && u <= 'Z') ||
                                  (u >= 'a' && u <= 'z') ||
                                  (u >= '0' && u <= '9') ||
                                  u == '+' || u == '/' || u == '=';
        if (!isBase64Char) {
            return false;
        }
        ++usefulChars;
    }

    if (usefulChars < 48 || (usefulChars % 4) != 0) {
        return false;
    }

    QByteArray decoded = QByteArray::fromBase64(text.toLatin1());
    if (decoded.size() < 64) {
        return false;
    }

    QImage image;
    return image.loadFromData(decoded);
}

} // namespace

struct EmojiDescriptor {
    QString glyph;
    QString name;
    QString iconPath;
};

class EmojiPickerWidget : public QWidget
{
    Q_OBJECT

public:
    using Category = QList<EmojiDescriptor>;
    using CategoryMap = QList<QPair<QString, Category>>;

    explicit EmojiPickerWidget(const CategoryMap& categories, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(6);

        auto* tabs = new QTabWidget(this);
        tabs->setTabPosition(QTabWidget::North);
        tabs->setDocumentMode(true);
        tabs->setElideMode(Qt::ElideRight);

        for (const auto& pair : categories) {
            QWidget* page = createCategoryPage(pair.second);
            tabs->addTab(page, pair.first);
        }

        layout->addWidget(tabs);
        setMinimumWidth(360);
        setMinimumHeight(240);
    }

signals:
    void emojiSelected(const QString& glyph);

private:
    QWidget* createCategoryPage(const Category& items)
    {
        auto* container = new QWidget;
        auto* grid = new QGridLayout(container);
        grid->setContentsMargins(8, 8, 8, 8);
        grid->setHorizontalSpacing(8);
        grid->setVerticalSpacing(4);

        constexpr int kColumns = 6;
        for (int index = 0; index < items.size(); ++index) {
            const auto& item = items.at(index);
            int row = index / kColumns;
            int column = index % kColumns;

            auto* button = new QToolButton(container);
            button->setAutoRaise(true);
            button->setCheckable(false);
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            button->setIconSize(QSize(36, 36));
            button->setFixedSize(80, 90);
            button->setText(item.name);
            button->setFont(QFont(button->font().family(), button->font().pointSize(), QFont::Normal));
            if (!item.iconPath.isEmpty()) {
                button->setIcon(QIcon(item.iconPath));
            } else {
                button->setText(QStringLiteral("%1\n%2").arg(item.glyph, item.name));
            }
            button->setProperty("emojiGlyph", item.glyph);
            button->setToolTip(QStringLiteral("%1 %2").arg(item.glyph, item.name));

            connect(button, &QToolButton::clicked, this, [this, button]() {
                emit emojiSelected(button->property("emojiGlyph").toString());
            });

            grid->addWidget(button, row, column);
        }

        auto* area = new QScrollArea;
        area->setWidget(container);
        area->setWidgetResizable(true);
        area->setFrameShape(QFrame::NoFrame);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        return area;
    }
};

ChatWindow::ChatWindow(const UserInfo& peer_info, QWidget* parent)
    : QWidget(parent)
    , m_peer_info(peer_info)
    , m_group_id()
    , m_group_info()
    , m_member_list_widget(nullptr)
    , m_network_manager(nullptr)
    , m_message_display(nullptr)
    , m_input_edit(nullptr)
    , m_send_button(nullptr)
    , m_toolbar(nullptr)
    , m_peer_name_label(nullptr)
    , m_typing_indicator(nullptr)
    , m_emoji_menu(nullptr)
    , m_emoji_button(nullptr)
    , m_smart_reply_widget(nullptr)
    , m_smart_reply_layout(nullptr)
    , m_typing_timer(new QTimer(this))
    , m_is_typing(false)
    , m_is_ai_chat(peer_info.user_id == "ai_assistant")
    , m_is_loopback(peer_info.user_id == "loopback")
    , m_auto_image_download_enabled(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::Window, true);
    setWindowIcon(QIcon(":/icons/app_icon.svg"));
    setWindowTitle(QStringLiteral("聊天 - %1").arg(peerDisplayName()));

    m_emoji_resource_map.insert(QStringLiteral("😀"), QStringLiteral(":/emojis/google/grinning.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😁"), QStringLiteral(":/emojis/google/beaming.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😂"), QStringLiteral(":/emojis/google/joy.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🤣"), QStringLiteral(":/emojis/google/rofl.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😊"), QStringLiteral(":/emojis/google/smiling.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😍"), QStringLiteral(":/emojis/google/heart_eyes.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😘"), QStringLiteral(":/emojis/google/kissing_heart.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😎"), QStringLiteral(":/emojis/google/sunglasses.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😢"), QStringLiteral(":/emojis/google/crying.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😭"), QStringLiteral(":/emojis/google/sob.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😡"), QStringLiteral(":/emojis/google/angry.svg"));
    m_emoji_resource_map.insert(QStringLiteral("👍"), QStringLiteral(":/emojis/google/thumbs_up.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🙏"), QStringLiteral(":/emojis/google/pray.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🎉"), QStringLiteral(":/emojis/google/party.svg"));
    m_emoji_resource_map.insert(QStringLiteral("❤️"), QStringLiteral(":/emojis/google/heart.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🔥"), QStringLiteral(":/emojis/google/fire.svg"));
    m_emoji_resource_map.insert(QStringLiteral("✨"), QStringLiteral(":/emojis/google/sparkles.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🥰"), QStringLiteral(":/emojis/google/smiling_hearts.svg"));

    setupUI();
    setupConnections();

    m_message_repository = Core::DI::ServiceLocator::instance()
                               .resolve<Core::Repositories::MessageRepository>();

    resize(720, 560);
}

ChatWindow::ChatWindow(const Core::GroupInfo& group_info, QWidget* parent)
    : QWidget(parent)
    , m_peer_info()
    , m_group_id(group_info.group_id)
    , m_group_info(group_info)
    , m_member_list_widget(nullptr)
    , m_network_manager(nullptr)
    , m_message_display(nullptr)
    , m_input_edit(nullptr)
    , m_send_button(nullptr)
    , m_toolbar(nullptr)
    , m_peer_name_label(nullptr)
    , m_typing_indicator(nullptr)
    , m_emoji_menu(nullptr)
    , m_emoji_button(nullptr)
    , m_smart_reply_widget(nullptr)
    , m_smart_reply_layout(nullptr)
    , m_typing_timer(new QTimer(this))
    , m_is_typing(false)
    , m_is_ai_chat(false)
    , m_is_loopback(false)
    , m_auto_image_download_enabled(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::Window, true);
    setWindowIcon(QIcon(":/icons/app_icon.svg"));
    setWindowTitle(QStringLiteral("群组 - %1").arg(group_info.group_name));

    // 初始化表情资源映射（与单聊相同）
    m_emoji_resource_map.insert(QStringLiteral("😀"), QStringLiteral(":/emojis/google/grinning.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😁"), QStringLiteral(":/emojis/google/beaming.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😂"), QStringLiteral(":/emojis/google/joy.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🤣"), QStringLiteral(":/emojis/google/rofl.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😊"), QStringLiteral(":/emojis/google/smiling.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😍"), QStringLiteral(":/emojis/google/heart_eyes.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😘"), QStringLiteral(":/emojis/google/kissing_heart.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😎"), QStringLiteral(":/emojis/google/sunglasses.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😢"), QStringLiteral(":/emojis/google/crying.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😭"), QStringLiteral(":/emojis/google/sob.svg"));
    m_emoji_resource_map.insert(QStringLiteral("😡"), QStringLiteral(":/emojis/google/angry.svg"));
    m_emoji_resource_map.insert(QStringLiteral("👍"), QStringLiteral(":/emojis/google/thumbs_up.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🙏"), QStringLiteral(":/emojis/google/pray.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🎉"), QStringLiteral(":/emojis/google/party.svg"));
    m_emoji_resource_map.insert(QStringLiteral("❤️"), QStringLiteral(":/emojis/google/heart.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🔥"), QStringLiteral(":/emojis/google/fire.svg"));
    m_emoji_resource_map.insert(QStringLiteral("✨"), QStringLiteral(":/emojis/google/sparkles.svg"));
    m_emoji_resource_map.insert(QStringLiteral("🥰"), QStringLiteral(":/emojis/google/smiling_hearts.svg"));

    setupUI();
    setupConnections();

    m_message_repository = Core::DI::ServiceLocator::instance()
                               .resolve<Core::Repositories::MessageRepository>();

    resize(720, 560);
}

ChatWindow::~ChatWindow()
{
}

void ChatWindow::setNetworkManager(NetworkManager* network_manager)
{
    if (m_network_manager == network_manager) {
        if (m_network_manager) {
            m_local_user_id = m_network_manager->getLocalUserId();
        }
        return;
    }

    if (m_network_manager) {
        QObject::disconnect(m_network_manager, nullptr, this, nullptr);
    }

    m_network_manager = network_manager;
    if (m_network_manager) {
        m_local_user_id = m_network_manager->getLocalUserId();
        connect(m_network_manager, &NetworkManager::fileOfferReceived,
                this, &ChatWindow::onNetworkFileOffer, Qt::UniqueConnection);
        connect(m_network_manager, &NetworkManager::fileTransferProgress,
                this, &ChatWindow::onNetworkFileTransferProgress, Qt::UniqueConnection);
        connect(m_network_manager, &NetworkManager::fileTransferFinished,
                this, &ChatWindow::onNetworkFileTransferFinished, Qt::UniqueConnection);
        connect(m_network_manager, &NetworkManager::fileTransferFailed,
                this, &ChatWindow::onNetworkFileTransferFailed, Qt::UniqueConnection);
    } else {
        m_local_user_id.clear();
    }
}

void ChatWindow::setAutoImageDownloadConfig(bool enabled, const QString& directory)
{
    const QString trimmed = directory.trimmed();
    if (enabled && trimmed.isEmpty()) {
        m_auto_image_download_enabled = false;
        m_auto_image_download_dir.clear();
        qCWarning(lcChatWindow) << "Auto image download disabled because directory is empty";
        return;
    }

    if (enabled) {
        QDir dir(trimmed);
        m_auto_image_download_enabled = true;
        m_auto_image_download_dir = dir.absolutePath();
        qCInfo(lcChatWindow) << "Auto image download enabled" << m_auto_image_download_dir;
    } else {
        m_auto_image_download_enabled = false;
        m_auto_image_download_dir.clear();
        qCInfo(lcChatWindow) << "Auto image download disabled";
    }

    if (!m_auto_image_download_enabled) {
        return;
    }

    for (auto it = m_pending_transfers.constBegin(); it != m_pending_transfers.constEnd(); ++it) {
        const PendingTransfer& transfer = it.value();
        if (transfer.is_outgoing || transfer.accepted) {
            continue;
        }
        qCDebug(lcChatWindow) << "Evaluating pending transfer for auto-accept during config apply"
                              << transfer.message_id
                              << transfer.filename
                              << transfer.packet_no
                              << transfer.file_id;
        maybeAutoAcceptImageTransfer(it.key());
    }
}

void ChatWindow::setAIService(std::shared_ptr<IAIService> ai_service)
{
    m_ai_service = ai_service;
}

void ChatWindow::setComplianceService(std::shared_ptr<IComplianceService> compliance_service)
{
    m_compliance_service = compliance_service;
}

// ============================================================================
// UI设置
// ============================================================================

void ChatWindow::setupUI()
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    
    // 顶部工具栏
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(20, 20));
    
    // 对方信息
    QWidget* identity_widget = new QWidget(this);
    QHBoxLayout* identity_layout = new QHBoxLayout(identity_widget);
    identity_layout->setContentsMargins(0, 0, 0, 0);
    identity_layout->setSpacing(6);

    m_presence_icon = new QLabel(this);
    m_presence_icon->setFixedSize(18, 18);
    identity_layout->addWidget(m_presence_icon);

    m_peer_name_label = new QLabel(this);
    m_peer_name_label->setStyleSheet(
        "QLabel { font-weight: bold; font-size: 14px; }"
    );
    identity_layout->addWidget(m_peer_name_label);
    identity_layout->addStretch();

    m_toolbar->addWidget(identity_widget);

    updateWindowTitle();
    updatePresenceIcon();
    updatePresenceTooltip();
    
    m_toolbar->addSeparator();
    
    // 工具按钮
    auto file_action = m_toolbar->addAction(actionIcon("file"), "发送文件", this, &ChatWindow::onSendFile);
    file_action->setToolTip("发送文件");
    auto camera_action = m_toolbar->addAction(actionIcon("camera"), "截图", this, &ChatWindow::onSendScreenshot);
    camera_action->setToolTip("截取屏幕");
    auto emoji_button = new QToolButton(this);
    emoji_button->setIcon(actionIcon("smile"));
    emoji_button->setToolTip("插入表情");
    emoji_button->setPopupMode(QToolButton::InstantPopup);
    m_toolbar->addWidget(emoji_button);
    m_emoji_button = emoji_button;
    
    main_layout->addWidget(m_toolbar);
    
    // 打字指示器
    m_typing_indicator = new QLabel("", this);
    m_typing_indicator->setStyleSheet(
        "QLabel { color: gray; font-style: italic; padding: 2px 10px; }");
    m_typing_indicator->hide();
    main_layout->addWidget(m_typing_indicator);
    
    // 消息显示区域
    m_message_display = new QTextBrowser(this);
    m_message_display->setOpenLinks(false);
    m_message_display->setOpenExternalLinks(false);
    m_message_display->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_message_display->setStyleSheet(
        "QTextBrowser {"
        "  border: 1px solid #ddd;"
        "  background-color: #f5f5f5;"
        "  padding: 10px;"
        "}");
    main_layout->addWidget(m_message_display);
    
    // 智能回复区域
    m_smart_reply_widget = new QWidget(this);
    m_smart_reply_layout = new QHBoxLayout(m_smart_reply_widget);
    m_smart_reply_layout->setContentsMargins(5, 5, 5, 5);
    
    QLabel* reply_label = new QLabel("💡 智能回复:", this);
    m_smart_reply_layout->addWidget(reply_label);
    m_smart_reply_layout->addStretch();
    
    m_smart_reply_widget->hide();
    main_layout->addWidget(m_smart_reply_widget);
    
    // 输入区域
    QHBoxLayout* input_layout = new QHBoxLayout();
    input_layout->setContentsMargins(5, 5, 5, 5);
    
    m_input_edit = new QLineEdit(this);
    m_input_edit->setPlaceholderText("输入消息...");
    m_input_edit->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #ddd;"
        "  border-radius: 3px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "}");
    input_layout->addWidget(m_input_edit);
    
    m_send_button = new QPushButton(actionIcon("message"), "发送", this);
    m_send_button->setFixedWidth(80);
    m_send_button->setStyleSheet(
        "QPushButton {"
        "  background-color: #0078d4;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 3px;"
        "  padding: 8px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #005a9e;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #004578;"
        "}");
    input_layout->addWidget(m_send_button);
    
    main_layout->addLayout(input_layout);
    
    setLayout(main_layout);

    // 初始化表情菜单
    m_emoji_menu = new QMenu(this);
    EmojiPickerWidget::CategoryMap categories;

    const struct {
        const char* glyph;
        const char* name;
        const char* category;
    } definitions[] = {
        {"😀", "微笑", "笑脸"},
        {"😁", "露齿笑", "笑脸"},
        {"😂", "喜极而泣", "笑脸"},
        {"🤣", "笑到打滚", "笑脸"},
        {"😊", "甜笑", "笑脸"},
        {"😍", "迷恋", "笑脸"},
        {"🥰", "爱心笑", "笑脸"},
        {"😘", "飞吻", "情感"},
        {"😢", "难过", "情感"},
        {"😭", "大哭", "情感"},
        {"😡", "愤怒", "情感"},
        {"❤️", "爱心", "情感"},
        {"👍", "点赞", "手势"},
        {"🙏", "祈祷", "手势"},
        {"😎", "酷", "手势"},
        {"🎉", "庆祝", "庆祝"},
        {"🔥", "火焰", "庆祝"},
        {"✨", "闪耀", "庆祝"}
    };

    QMap<QString, EmojiPickerWidget::Category> category_map;
    QStringList category_order;
    for (const auto& def : definitions) {
        QString category = QString::fromUtf8(def.category);
        if (!category_order.contains(category)) {
            category_order.append(category);
        }
        EmojiDescriptor descriptor;
        descriptor.glyph = QString::fromUtf8(def.glyph);
        descriptor.name = QString::fromUtf8(def.name);
        descriptor.iconPath = m_emoji_resource_map.value(descriptor.glyph);
        category_map[category].append(descriptor);
    }

    for (const QString& category : category_order) {
        categories.append(qMakePair(category, category_map.value(category)));
    }

    auto* picker = new EmojiPickerWidget(categories, m_emoji_menu);
    connect(picker, &EmojiPickerWidget::emojiSelected, this, [this](const QString& emoji) {
        sendEmojiMessage(emoji);
    });

    auto* widget_action = new QWidgetAction(m_emoji_menu);
    widget_action->setDefaultWidget(picker);
    m_emoji_menu->addAction(widget_action);
    emoji_button->setMenu(m_emoji_menu);
}

void ChatWindow::setupConnections()
{
    // 发送按钮
    connect(m_send_button, &QPushButton::clicked,
            this, &ChatWindow::onSendMessage);
    
    // 回车发送
    connect(m_input_edit, &QLineEdit::returnPressed,
            this, &ChatWindow::onSendMessage);
    
    // 输入变化
    connect(m_input_edit, &QLineEdit::textChanged,
            this, &ChatWindow::onInputTextChanged);
    
    // 打字定时器
    m_typing_timer->setSingleShot(true);
    connect(m_typing_timer, &QTimer::timeout,
            this, &ChatWindow::onTypingTimeout);

    if (m_emoji_button) {
        connect(m_emoji_button, &QToolButton::clicked,
                this, &ChatWindow::onSendEmoji);
    }

    connect(m_message_display, &QTextBrowser::anchorClicked,
            this, &ChatWindow::onMessageLinkActivated);
}

void ChatWindow::updatePeerInfo(const UserInfo& info)
{
    m_peer_info = info;
    updateWindowTitle();
    updatePresenceIcon();
    updatePresenceTooltip();
}

void ChatWindow::updateGroupInfo(const Core::GroupInfo& info)
{
    m_group_info = info;
    m_group_id = info.group_id;
    updateWindowTitle();
}

void ChatWindow::updateGroupMembers(const QList<Core::UserInfo>& members)
{
    m_group_members = members;
    // TODO: 更新成员列表UI
}

// ============================================================================
// 消息处理
// ============================================================================

void ChatWindow::onSendMessage()
{
    QString text = m_input_edit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    
    m_input_edit->clear();
    
    // 如果是AI聊天
    if (m_is_ai_chat && m_ai_service) {
        // 显示用户消息
    ChatMessage user_msg;
    user_msg.message_id = QUuid::createUuid().toString();
    user_msg.sender_id = QStringLiteral("user");
    user_msg.content = text;
    user_msg.timestamp = QDateTime::currentDateTime();
        
        addMessageToDisplay(user_msg, true);
        m_message_history.push_back(user_msg);
        persistMessage(user_msg);
        
        // 显示"AI正在思考..."
        m_message_display->append(
            "<div style='color: gray; font-style: italic;'>"
            "AI正在思考..."
            "</div>");
        
        // 异步处理AI响应
    std::string input = text.toStdString();
        m_ai_service->processTextAsync(input, 
            [this](const AIResult& result) {
                // 在主线程中更新UI
                QMetaObject::invokeMethod(this, [this, result]() {
                    // 移除"正在思考"提示
                    QString html = m_message_display->toHtml();
                    int pos = html.lastIndexOf("AI正在思考...");
                    if (pos != -1) {
                        m_message_display->clear();
                        html.remove(pos - 50, 100); // 粗略移除
                        m_message_display->setHtml(html);
                    }
                    
                    // 显示AI响应
                    ChatMessage ai_msg;
                    ai_msg.message_id = QUuid::createUuid().toString();
                    ai_msg.sender_id = QStringLiteral("ai_assistant");
                    ai_msg.content = QString::fromStdString(result.text_output);
                    ai_msg.timestamp = QDateTime::currentDateTime();
                    
                    addMessageToDisplay(ai_msg, false);
                    m_message_history.push_back(ai_msg);
                    persistMessage(ai_msg);
                    
                    // 生成智能回复建议
                    if (m_ai_service->getCapabilities() & AICapability::SmartReply) {
                        std::vector<std::string> history_text;
                        for (const auto& msg : m_message_history) {
                            history_text.push_back(msg.content.toStdString());
                        }
                        
                        auto reply_result = m_ai_service->generateSmartReplies(
                            history_text, 3);
                        
                        if (reply_result.success && 
                            !reply_result.suggestions.empty()) {
                            displaySmartReplies(reply_result.suggestions);
                        }
                    }
                }, Qt::QueuedConnection);
            });
    }
    else {
        // 普通P2P消息
        sendTextMessage(text);
    }
    
    // 清除智能回复
    clearSmartReplies();
}

void ChatWindow::sendTextMessage(const QString& text)
{
    if (!m_network_manager) {
        QMessageBox::warning(this, "错误", "网络管理器未初始化");
        return;
    }
    
    CompliancePayload payload;
    payload.target = ComplianceTarget::Text;
    payload.conversation_id = m_peer_info.user_id;
    payload.local_user_id = m_network_manager->getLocalUserId();
    payload.peer_user_id = m_peer_info.user_id;
    payload.message_id = QUuid::createUuid().toString();
    payload.text = text;
    payload.content_type = QStringLiteral("text/plain");
    payload.is_outgoing = true;

    ComplianceResult compliance;
    if (!evaluateCompliance(payload, compliance)) {
        QMessageBox::warning(this, "提示", compliance.reason.isEmpty() ? QStringLiteral("消息发送已被合规策略阻止") : compliance.reason);
        return;
    }

    if (compliance.verdict == ComplianceVerdict::NeedsReview) {
        appendSystemMessage(compliance.reason.isEmpty() ? QStringLiteral("消息已发送，但被标记为需合规复核。") : compliance.reason);
    }

    ChatMessage message;
    message.message_id = payload.message_id;
    QString sender_id = payload.local_user_id;
    message.sender_id = sender_id;
    // 群组消息或单聊消息
    if (isGroupChat()) {
        message.group_id = m_group_id;
        message.receiver_id = QString();  // 群组消息没有特定接收者
        message.message_type = MessageContentType::PlainText;
        message.content = text;
        message.timestamp = QDateTime::currentDateTime();

        if (m_network_manager->sendGroupMessage(m_group_id, message)) {
            addMessageToDisplay(message, true);
            m_message_history.push_back(message);
            persistMessage(message);
        } else {
            QMessageBox::warning(this, "错误", "群组消息发送失败");
        }
    } else {
        message.receiver_id = m_peer_info.user_id;
        message.message_type = MessageContentType::PlainText;
        message.content = text;
        message.timestamp = QDateTime::currentDateTime();

        if (m_network_manager->sendMessage(m_peer_info.user_id, message)) {
            addMessageToDisplay(message, true);
            m_message_history.push_back(message);
            persistMessage(message);
        } else {
            QMessageBox::warning(this, "错误", "消息发送失败");
        }
    }
}

void ChatWindow::sendImageMessage(const QImage& image)
{
    if (!m_network_manager) {
        return;
    }
    
    // 将图像转换为Base64
    QByteArray image_data;
    QBuffer buffer(&image_data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    
    CompliancePayload payload;
    payload.target = ComplianceTarget::Image;
    payload.conversation_id = m_peer_info.user_id;
    payload.local_user_id = m_network_manager->getLocalUserId();
    payload.peer_user_id = m_peer_info.user_id;
    payload.message_id = QUuid::createUuid().toString();
    payload.binary = image_data;
    payload.content_type = QStringLiteral("image/png");
    payload.is_outgoing = true;

    ComplianceResult compliance;
    if (!evaluateCompliance(payload, compliance)) {
        QMessageBox::warning(this, "提示", compliance.reason.isEmpty() ? QStringLiteral("图片发送已被合规策略阻止") : compliance.reason);
        return;
    }

    if (compliance.verdict == ComplianceVerdict::NeedsReview) {
        appendSystemMessage(compliance.reason.isEmpty() ? QStringLiteral("图片已发送，但被标记为需合规复核。") : compliance.reason);
    }

    const QString verdict_code = verdictCodeFromResult(compliance.verdict);
    const QString verdict_reason = compliance.reason.trimmed();

    QString base64_image = QString::fromLatin1(image_data.toBase64());
    
    ChatMessage message;
    message.message_id = payload.message_id;
    QString sender_id = payload.local_user_id;
    message.sender_id = sender_id;
    message.receiver_id = m_peer_info.user_id;
    message.message_type = MessageContentType::Image;
    message.content = base64_image;
    message.timestamp = QDateTime::currentDateTime();
    
    if (!verdict_code.isEmpty()) {
        message.metadata.insert(QLatin1String(kComplianceVerdictKey), verdict_code);
        if (!verdict_reason.isEmpty()) {
            message.metadata.insert(QLatin1String(kComplianceReasonKey), verdict_reason);
        }
    }
    
    ChatMessage outbound = message;
    outbound.content = Network::PayloadTags::applyImagePrefix(base64_image);

    if (m_network_manager->sendMessage(m_peer_info.user_id, outbound)) {
    addMessageToDisplay(message, true);
    m_message_history.push_back(message);
        persistMessage(message);
    } else {
        QMessageBox::warning(this, "错误", "图片发送失败");
    }
}

void ChatWindow::sendEmojiMessage(const QString& emoji)
{
    if (emoji.isEmpty()) {
        return;
    }

    if (m_is_ai_chat && m_ai_service) {
        m_input_edit->setText(emoji);
        onSendMessage();
        return;
    }

    CompliancePayload payload;
    payload.target = ComplianceTarget::Text;
    payload.conversation_id = m_peer_info.user_id;
    payload.local_user_id = m_network_manager ? m_network_manager->getLocalUserId() : QStringLiteral("local");
    payload.peer_user_id = m_peer_info.user_id;
    payload.message_id = QUuid::createUuid().toString();
    payload.text = emoji;
    payload.content_type = QStringLiteral("text/plain");
    payload.is_outgoing = true;

    ComplianceResult compliance;
    if (!evaluateCompliance(payload, compliance)) {
        QMessageBox::warning(this, "提示", compliance.reason.isEmpty() ? QStringLiteral("表情发送已被合规策略阻止") : compliance.reason);
        return;
    }

    if (compliance.verdict == ComplianceVerdict::NeedsReview) {
        appendSystemMessage(compliance.reason.isEmpty() ? QStringLiteral("表情已发送，但被标记为需合规复核。") : compliance.reason);
    }

    ChatMessage message;
    message.message_id = payload.message_id;
    QString sender_id = payload.local_user_id;
    message.sender_id = sender_id;
    message.receiver_id = m_peer_info.user_id;
    message.message_type = MessageContentType::Emoji;
    message.content = emoji;
    message.timestamp = QDateTime::currentDateTime();

    if (m_network_manager) {
        if (m_network_manager->sendMessage(m_peer_info.user_id, message)) {
            addMessageToDisplay(message, true);
            m_message_history.push_back(message);
            persistMessage(message);
        } else {
            QMessageBox::warning(this, "错误", "表情发送失败");
        }
    } else {
        addMessageToDisplay(message, true);
        m_message_history.push_back(message);
        persistMessage(message);
    }
}

void ChatWindow::addReceivedMessage(const ChatMessage& message)
{
    CompliancePayload payload;
    payload.conversation_id = m_peer_info.user_id;
    payload.local_user_id = m_network_manager ? m_network_manager->getLocalUserId() : QString();
    payload.peer_user_id = message.sender_id;
    payload.message_id = message.message_id;
    payload.is_outgoing = false;

    switch (message.message_type) {
        case MessageContentType::PlainText:
            payload.target = ComplianceTarget::Text;
            payload.text = message.content;
            payload.content_type = QStringLiteral("text/plain");
            break;
        case MessageContentType::Emoji:
            payload.target = ComplianceTarget::Text;
            payload.text = message.content;
            payload.content_type = QStringLiteral("text/plain");
            break;
        case MessageContentType::Image:
        {
            payload.target = ComplianceTarget::Image;
            QByteArray binary = QByteArray::fromBase64(message.content.toLatin1());
            payload.binary = binary;
            payload.content_type = QStringLiteral("image/png");
            break;
        }
        case MessageContentType::File:
        {
            payload.target = ComplianceTarget::File;
            QString data_b64 = message.metadata.value(QStringLiteral("data"));
            if (!data_b64.isEmpty()) {
                payload.binary = QByteArray::fromBase64(data_b64.toLatin1());
            }
            payload.text = message.content;
            payload.content_type = QStringLiteral("application/octet-stream");
            break;
        }
        default:
            payload.target = ComplianceTarget::Text;
            payload.text = message.content;
            break;
    }

    ComplianceResult compliance;
    if (!evaluateCompliance(payload, compliance)) {
        appendSystemMessage(compliance.reason.isEmpty() ? QStringLiteral("收到的消息已被合规策略阻止。") : compliance.reason);
        return;
    }

    if (compliance.verdict == ComplianceVerdict::NeedsReview) {
        appendSystemMessage(compliance.reason.isEmpty() ? QStringLiteral("收到的消息需合规复核。") : compliance.reason);
    }

    ChatMessage enriched = message;
    const QString verdict_code = verdictCodeFromResult(compliance.verdict);
    const QString verdict_reason = compliance.reason.trimmed();
    if (!verdict_code.isEmpty() && message.message_type == MessageContentType::Image) {
        enriched.metadata.insert(QLatin1String(kComplianceVerdictKey), verdict_code);
        if (!verdict_reason.isEmpty()) {
            enriched.metadata.insert(QLatin1String(kComplianceReasonKey), verdict_reason);
        }
    }

    addMessageToDisplay(enriched, false);
    m_message_history.push_back(enriched);
    
    // 生成智能回复
    if (m_ai_service && 
        (m_ai_service->getCapabilities() & AICapability::SmartReply)) {
        std::vector<std::string> history_text;
        for (const auto& msg : m_message_history) {
            history_text.push_back(msg.content.toStdString());
        }
        
        auto result = m_ai_service->generateSmartReplies(history_text, 3);
        if (result.success && !result.suggestions.empty()) {
            displaySmartReplies(result.suggestions);
        }
    }
}

void ChatWindow::appendHistoryMessage(const ChatMessage& message, bool is_outgoing)
{
    addMessageToDisplay(message, is_outgoing);
    m_message_history.push_back(message);
}

void ChatWindow::addMessageToDisplay(const ChatMessage& message, bool is_sent)
{
    if (message.message_type == MessageContentType::File) {
        registerFileTransfer(message);
    }

    QString time_str = formatMessageTime(message.timestamp);
    QString content = formatMessageContent(message);

    const QString bg_color = is_sent ? "#dcf8c6" : "#ffffff";
    const QString content_alignment = is_sent ? "right" : "left";
    const QString time_alignment = is_sent ? "right" : "left";

    QString compliance_html;
    if (message.message_type == MessageContentType::Image) {
        const QString verdict_code = message.metadata.value(QLatin1String(kComplianceVerdictKey));
        if (!verdict_code.isEmpty()) {
            const QString reason = message.metadata.value(QLatin1String(kComplianceReasonKey));
            const QString note = verdictNoteForImage(verdict_code, reason);
            if (!note.isEmpty()) {
                const QString color = verdictColorForImage(verdict_code);
                compliance_html = QStringLiteral(
                    "  <div style='color:%1; font-size:11px; margin:6px 0 0 0; text-align:%2;'>%3</div>")
                                       .arg(color, content_alignment, note.toHtmlEscaped());
            }
        }
    }

    QString name = QStringLiteral("我");
    if (!is_sent) {
        name = peerDisplayName();
        if (!m_peer_info.ip_address.isEmpty() && m_peer_info.ip_address != name) {
            name += QStringLiteral(" [%1]").arg(m_peer_info.ip_address);
        }
    }
    name = name.toHtmlEscaped();

    const QString bubble = QString(
        "<div style='"
        "  background-color:%1;"
        "  border-radius:10px;"
        "  padding:10px 15px;"
        "  max-width:75%%;"
        "  box-shadow:0 1px 2px rgba(0,0,0,0.1);"
        "  display:inline-block;"
        "'>"
        "  <div style='font-weight:bold; color:#333; margin:0 0 5px 0; text-align:%2;'>%3</div>"
        "  <div style='color:#000; word-wrap:break-word; white-space:pre-wrap; text-align:%2;'>%4</div>"
        "%7"
        "  <div style='color:#888; font-size:11px; margin:5px 0 0 0; text-align:%5;'>%6</div>"
        "</div>")
        .arg(bg_color)
        .arg(content_alignment)
        .arg(name)
        .arg(content)
        .arg(time_alignment)
        .arg(time_str.toHtmlEscaped())
        .arg(compliance_html);

    QTextCursor cursor = m_message_display->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (!m_message_display->document()->isEmpty()) {
        cursor.insertBlock();
    }

    QTextBlockFormat block_format = cursor.blockFormat();
    block_format.setAlignment(is_sent ? Qt::AlignRight : Qt::AlignLeft);
    block_format.setBottomMargin(8);
    block_format.setTopMargin(8);
    block_format.setLeftMargin(is_sent ? 80 : 12);
    block_format.setRightMargin(is_sent ? 12 : 80);
    cursor.setBlockFormat(block_format);

    cursor.insertHtml(bubble);
    m_message_display->setTextCursor(cursor);

    // 滚动到底部
    QScrollBar* scrollbar = m_message_display->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

// ============================================================================
// 工具栏动作
// ============================================================================

void ChatWindow::onSendFile()
{
    QString filepath = QFileDialog::getOpenFileName(
        this, "选择文件", "", "所有文件 (*)");
    
    if (filepath.isEmpty()) {
        return;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开所选文件。");
        return;
    }

    QFileInfo file_info(filepath);
    QByteArray binary = file.readAll();
    file.close();

    QByteArray hash = QCryptographicHash::hash(binary, QCryptographicHash::Sha256);

    CompliancePayload payload;
    payload.target = ComplianceTarget::File;
    payload.conversation_id = m_peer_info.user_id;
    payload.local_user_id = m_network_manager->getLocalUserId();
    payload.peer_user_id = m_peer_info.user_id;
    payload.message_id = QUuid::createUuid().toString();
    payload.binary = binary;
    payload.content_type = QStringLiteral("application/octet-stream");
    payload.text = file_info.fileName();
    payload.is_outgoing = true;

    ComplianceResult compliance;
    if (!evaluateCompliance(payload, compliance)) {
        QMessageBox::warning(this, "提示", compliance.reason.isEmpty() ? QStringLiteral("文件发送已被合规策略阻止") : compliance.reason);
        return;
    }

    if (compliance.verdict == ComplianceVerdict::NeedsReview) {
        appendSystemMessage(compliance.reason.isEmpty() ? QStringLiteral("文件已发送，但被标记为需合规复核。") : compliance.reason);
    }

    ChatMessage message;
    message.message_id = payload.message_id;
    QString sender_id = payload.local_user_id;
    message.sender_id = sender_id;
    message.receiver_id = m_peer_info.user_id;
    message.message_type = MessageContentType::File;
    message.content = file_info.fileName();
    message.timestamp = QDateTime::currentDateTime();
    message.metadata[QStringLiteral("file_name")] = file_info.fileName();
    message.metadata[QStringLiteral("size")] = QString::number(file_info.size());
    message.metadata[QStringLiteral("hash")] = QString::fromLatin1(hash.toHex());
    message.metadata[QStringLiteral("local_path")] = QDir::toNativeSeparators(file_info.absoluteFilePath());
    message.metadata[QStringLiteral("transfer_status")] = QStringLiteral("pending");

    const QString content_type = imageMimeType(file_info.fileName());
    if (!content_type.isEmpty()) {
        message.metadata[QStringLiteral("content_type")] = content_type;
    }

    if (!m_network_manager) {
        QMessageBox::warning(this, "错误", "网络管理器未初始化，无法发送文件。");
        return;
    }

    quint32 packet_no = 0;
    quint32 file_id = 0;
    if (!m_network_manager->sendFile(m_peer_info.user_id,
                                     file_info.absoluteFilePath(),
                                     &packet_no,
                                     &file_id)) {
        QMessageBox::warning(this, "错误", "文件发送失败，请稍后重试。");
        return;
    }

    if (packet_no != 0) {
        message.metadata.insert(QStringLiteral("packet_no"), QString::number(packet_no));
    }
    if (file_id != 0) {
        message.metadata.insert(QStringLiteral("file_id"), QString::number(file_id));
    }
    if (!m_peer_info.ip_address.isEmpty()) {
        message.metadata.insert(QStringLiteral("sender_ip"), m_peer_info.ip_address);
    }

    addMessageToDisplay(message, true);
    m_message_history.push_back(message);
    persistMessage(message);
    appendSystemMessage(QStringLiteral("已开始发送文件：%1")
                            .arg(message.content.toHtmlEscaped()));
}

void ChatWindow::onSendScreenshot()
{
    // TODO: 实现截图功能
    QMessageBox::information(this, "提示", "截图功能开发中...");
}

void ChatWindow::onSendEmoji()
{
    if (m_emoji_button && m_emoji_menu) {
        const QPoint global_pos = m_emoji_button->mapToGlobal(QPoint(0, m_emoji_button->height()));
        m_emoji_menu->popup(global_pos);
    }
}

// ============================================================================
// 输入处理
// ============================================================================

void ChatWindow::onInputTextChanged()
{
    if (!m_is_typing && m_network_manager) {
        m_is_typing = true;
        m_network_manager->sendTypingIndicator(m_peer_info.user_id, true);
    }
    
    // 重置定时器
    m_typing_timer->start(2000);
}

void ChatWindow::onTypingTimeout()
{
    if (m_is_typing && m_network_manager) {
        m_is_typing = false;
        m_network_manager->sendTypingIndicator(m_peer_info.user_id, false);
    }
}

// ============================================================================
// 智能回复
// ============================================================================

void ChatWindow::displaySmartReplies(
    const std::vector<std::string>& suggestions)
{
    clearSmartReplies();
    
    for (const auto& suggestion : suggestions) {
        QPushButton* reply_button = new QPushButton(
            QString::fromStdString(suggestion), this);
        
        reply_button->setStyleSheet(
            "QPushButton {"
            "  background-color: #e0e0e0;"
            "  border: 1px solid #bbb;"
            "  border-radius: 15px;"
            "  padding: 5px 15px;"
            "  margin: 2px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #d0d0d0;"
            "}");
        
        connect(reply_button, &QPushButton::clicked, this, [=]() {
            m_input_edit->setText(QString::fromStdString(suggestion));
            onSendMessage();
        });
        
        m_smart_reply_layout->addWidget(reply_button);
    }
    
    m_smart_reply_widget->show();
}

void ChatWindow::clearSmartReplies()
{
    // 移除所有回复按钮
    QLayoutItem* item;
    while ((item = m_smart_reply_layout->takeAt(1)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    m_smart_reply_widget->hide();
}

void ChatWindow::appendSystemMessage(const QString& text)
{
    if (!m_message_display) {
        return;
    }

    QString html = QStringLiteral(
        "<div style='color:#555; font-style:italic; text-align:center; margin:6px 0;'>%1</div>")
                         .arg(text.toHtmlEscaped());
    m_message_display->append(html);
}

// ============================================================================
// 辅助函数
// ============================================================================

QString ChatWindow::formatMessageTime(const QDateTime& time) const
{
    QDateTime now = QDateTime::currentDateTime();
    
    if (time.date() == now.date()) {
        return time.toString("hh:mm");
    } else if (time.date() == now.date().addDays(-1)) {
        return "昨天 " + time.toString("hh:mm");
    } else {
        return time.toString("MM-dd hh:mm");
    }
}

QString ChatWindow::formatMessageContent(const ChatMessage& message)
{
    switch (message.message_type) {
        case MessageContentType::PlainText:
        {
            const QString& raw = message.content;

            if (Network::PayloadTags::hasImagePrefix(raw) || looksLikeBase64Image(raw)) {
                ChatMessage copy = message;
                copy.message_type = MessageContentType::Image;
                copy.content = Network::PayloadTags::stripImagePrefix(raw);
                bool ok = false;
                QString html = renderInlineImageHtml(copy, &ok);
                if (ok) {
                    return html;
                }
            }

            QString escaped = raw.toHtmlEscaped();
            escaped.replace(
                QRegularExpression(QStringLiteral("(https?://[^\\s]+)")),
                QStringLiteral("<a href='\\1'>\\1</a>"));
            return renderEmojiHtml(escaped);
        }
            
        case MessageContentType::Image:
            return renderInlineImageHtml(message);
            
        case MessageContentType::File:
        {
            QString filename = message.metadata.value(QStringLiteral("file_name"));
            if (filename.isEmpty()) {
                filename = message.content;
            }
            if (filename.isEmpty()) {
                filename = QStringLiteral("未命名文件");
            }
            const QString escaped_filename = filename.toHtmlEscaped();

            const QString icon_html = QStringLiteral("<img src=':/icons/file.svg' width='16' height='16' style='vertical-align:middle;margin-right:6px;' />");
            QString link = QStringLiteral("<a href=\"download:%1\">%2</a>")
                                .arg(message.message_id, escaped_filename);

            QString size_text;
            bool ok = false;
            const quint64 size_value = message.metadata.value(QStringLiteral("size")).toULongLong(&ok);
            if (ok && size_value > 0) {
                size_text = formatFileSize(size_value);
            }

            QString extra;
            if (!size_text.isEmpty()) {
                extra = QStringLiteral(" <span style='color:#666; font-size:11px;'>(%1)</span>")
                            .arg(size_text.toHtmlEscaped());
            }

            QString supplement;
            const QString caption = message.metadata.value(QStringLiteral("caption"));
            if (!caption.isEmpty()) {
                supplement += QStringLiteral(
                    "<div style='margin-top:6px;color:#555;font-size:12px;'>%1</div>")
                                   .arg(caption.toHtmlEscaped());
            }

            const QString attachments = message.metadata.value(QStringLiteral("attachment_names"));
            if (!attachments.isEmpty()) {
                const QStringList names = attachments.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                if (!names.isEmpty()) {
                    QStringList bullet;
                    bullet.reserve(names.size());
                    for (const QString& name : names) {
                        bullet.append(QStringLiteral("• %1").arg(name.toHtmlEscaped()));
                    }
                    supplement += QStringLiteral(
                        "<div style='margin-top:6px;color:#555;font-size:12px;'>%1</div>")
                                       .arg(bullet.join(QStringLiteral("<br />")));
                }
            }

            QString preview_html;
            QString declared_mime = message.metadata.value(QStringLiteral("content_type"));
            if (declared_mime.isEmpty()) {
                declared_mime = imageMimeType(filename);
            }

            if (!declared_mime.isEmpty() && declared_mime.startsWith(QStringLiteral("image/"))) {
                QString preview_path = message.metadata.value(QStringLiteral("saved_path"));
                if (preview_path.isEmpty()) {
                    preview_path = message.metadata.value(QStringLiteral("local_path"));
                }

                bool image_ok = false;
                if (!preview_path.isEmpty()) {
                    preview_html = renderImageFromFile(preview_path,
                                                       message.message_id + QStringLiteral("-preview"),
                                                       &image_ok);
                }

                if ((!image_ok || preview_html.isEmpty())) {
                    const QString inline_data = message.metadata.value(QStringLiteral("data"));
                    if (!inline_data.isEmpty()) {
                        ChatMessage clone = message;
                        clone.message_type = MessageContentType::Image;
                        clone.content = inline_data;
                        preview_html = renderInlineImageHtml(clone, &image_ok);
                        if (!image_ok) {
                            preview_html.clear();
                        }
                    }
                }

                if (!preview_html.isEmpty()) {
                    preview_html.prepend(QStringLiteral("<div style='margin-top:8px;'>"));
                    preview_html.append(QStringLiteral("</div>"));
                }
            }

            return QStringLiteral("%1%2%3%4%5")
                .arg(icon_html, link, extra, supplement, preview_html);
        }
            
        case MessageContentType::Emoji:
            return emojiImageTag(message.content);
            
        default:
            return message.content.toHtmlEscaped();
    }
}

QString ChatWindow::renderInlineImageHtml(const ChatMessage& message, bool* ok)
{
    QString payload = Network::PayloadTags::stripImagePrefix(message.content).trimmed();
    if (payload.isEmpty()) {
        if (ok) {
            *ok = false;
        }
        return QStringLiteral("<div style='color:#c62828;'>[图片内容为空]</div>");
    }

    if (payload.startsWith(QStringLiteral("<img"), Qt::CaseInsensitive)) {
        if (ok) {
            *ok = true;
        }
        return payload;
    }

    QByteArray raw = QByteArray::fromBase64(payload.toLatin1());
    if (raw.isEmpty()) {
        const QString metadataData = message.metadata.value(QStringLiteral("data"));
        if (!metadataData.isEmpty()) {
            raw = QByteArray::fromBase64(metadataData.toLatin1());
        }
    }

    if (raw.isEmpty()) {
        if (ok) {
            *ok = false;
        }
        return QStringLiteral("<div style='color:#c62828;'>[图片加载失败]</div>");
    }

    QImage image;
    if (!image.loadFromData(raw)) {
        if (ok) {
            *ok = false;
        }
        return QStringLiteral("<div style='color:#c62828;'>[图片渲染失败]</div>");
    }

    if (!image.size().isValid()) {
        if (ok) {
            *ok = false;
        }
        return QStringLiteral("<div style='color:#c62828;'>[图片尺寸无效]</div>");
    }

    const QSize maxSize(320, 320);
    if (image.width() > maxSize.width() || image.height() > maxSize.height()) {
        image = image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QString key = message.message_id;
    if (key.isEmpty()) {
        key = QUuid::createUuid().toString(QUuid::WithoutBraces);
    } else {
        key.remove(QLatin1Char('{'));
        key.remove(QLatin1Char('}'));
    }

    const QString resourceId = QStringLiteral("internal:%1").arg(key);
    if (m_message_display && m_message_display->document()) {
        m_message_display->document()->addResource(QTextDocument::ImageResource,
                                                   QUrl(resourceId),
                                                   image);
    }

    if (ok) {
        *ok = true;
    }
    return QStringLiteral("<img src='%1' width='%2' height='%3' style='border-radius:6px; display:block;' alt='图片' />")
        .arg(resourceId,
             QString::number(image.width()),
             QString::number(image.height()));
}

QString ChatWindow::renderImageFromFile(const QString& file_path,
                                        const QString& cache_key,
                                        bool* ok)
{
    if (ok) {
        *ok = false;
    }

    if (file_path.isEmpty()) {
        return QString();
    }

    QFileInfo info(file_path);
    if (!info.exists() || !info.isFile()) {
        return QString();
    }

    QImage image(file_path);
    if (image.isNull()) {
        return QString();
    }

    const QSize maxSize(320, 320);
    if (image.width() > maxSize.width() || image.height() > maxSize.height()) {
        image = image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QString key = cache_key;
    if (key.isEmpty()) {
        key = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    key.remove(QLatin1Char('{'));
    key.remove(QLatin1Char('}'));

    const QString resourceId = QStringLiteral("internal:%1").arg(key);
    if (m_message_display && m_message_display->document()) {
        m_message_display->document()->addResource(QTextDocument::ImageResource,
                                                   QUrl(resourceId),
                                                   image);
    }

    if (ok) {
        *ok = true;
    }

    return QStringLiteral("<img src='%1' width='%2' height='%3' style='border-radius:6px; display:block;' alt='%4' />")
        .arg(resourceId,
             QString::number(image.width()),
             QString::number(image.height()),
             info.fileName().toHtmlEscaped());
}

QString ChatWindow::makeUniqueFilePath(const QString& directory, const QString& filename) const
{
    QDir dir(directory);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    QFileInfo info(filename);
    QString base = info.completeBaseName();
    QString suffix = info.suffix();
    if (base.isEmpty()) {
        base = QStringLiteral("image_%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    }

    QString candidate = dir.filePath(filename.isEmpty() ? base : filename);
    int counter = 1;
    while (QFileInfo::exists(candidate)) {
        QString numbered = base;
        numbered += QStringLiteral("_%1").arg(counter, 2, 10, QLatin1Char('0'));
        if (!suffix.isEmpty()) {
            numbered += QLatin1Char('.');
            numbered += suffix;
        }
        candidate = dir.filePath(numbered);
        ++counter;
        if (counter > 5000) {
            candidate = dir.filePath(base + QStringLiteral("_%1").arg(QDateTime::currentSecsSinceEpoch()));
            if (!suffix.isEmpty()) {
                candidate += QLatin1Char('.');
                candidate += suffix;
            }
            break;
        }
    }

    return QDir::toNativeSeparators(candidate);
}

void ChatWindow::applyPendingTransferMetadata(const QString& message_id,
                                              const QString& key,
                                              const QString& value,
                                              bool removed)
{
    if (!m_pending_transfers.contains(message_id)) {
        return;
    }

    PendingTransfer transfer = m_pending_transfers.value(message_id);

    if (key == QStringLiteral("saved_path")) {
        transfer.saved_path = removed ? QString() : value;
    } else if (key == QStringLiteral("local_path")) {
        transfer.local_path = removed ? QString() : value;
    } else if (key == QStringLiteral("size")) {
        if (!removed) {
            bool ok = false;
            const quint64 size = value.toULongLong(&ok);
            if (ok) {
                transfer.filesize = size;
            }
        }
    } else if (key == QStringLiteral("file_name")) {
        if (!removed && !value.isEmpty()) {
            transfer.filename = value;
        }
    } else if (key == QStringLiteral("transfer_status")) {
        const QString normalized = removed ? QString() : value;
        transfer.accepted = (normalized == QStringLiteral("in_progress"));
        transfer.completed = (normalized == QStringLiteral("completed"));
        if (normalized == QStringLiteral("failed") || normalized == QStringLiteral("offered")) {
            transfer.accepted = false;
        }
    } else if (key == QStringLiteral("content_type")) {
        transfer.content_type = removed ? QString() : value;
    }

    m_pending_transfers.insert(message_id, transfer);
}

void ChatWindow::maybeAutoAcceptImageTransfer(const QString& message_id)
{
    if (!m_auto_image_download_enabled || m_auto_image_download_dir.isEmpty()) {
        qCDebug(lcChatWindow) << "Skip auto-accept; feature disabled" << message_id;
        return;
    }
    if (!m_network_manager || !m_pending_transfers.contains(message_id)) {
        qCDebug(lcChatWindow) << "Skip auto-accept; missing network manager or transfer" << message_id;
        return;
    }

    PendingTransfer transfer = m_pending_transfers.value(message_id);
    
    // 修复：对于loopback场景，如果文件提供来自网络（通过onNetworkFileOffer接收），
    // 即使is_outgoing为true，也应该允许自动下载（因为这是通过网络接收的）
    bool should_allow_auto_accept = false;
    if (transfer.is_outgoing && m_is_loopback && transfer.peer_id == m_local_user_id) {
        // Loopback场景：检查消息历史，如果消息的receiver_id是local_user_id，说明这是接收的消息
        for (const ChatMessage& msg : m_message_history) {
            if (msg.message_id == message_id && msg.receiver_id == m_local_user_id) {
                // 这是通过网络接收的消息，应该允许自动下载
                should_allow_auto_accept = true;
                transfer.is_outgoing = false;  // 重置为false以允许自动下载
                m_pending_transfers.insert(message_id, transfer);
                qCDebug(lcChatWindow) << "Allow auto-accept for loopback received message" << message_id;
                break;
            }
        }
    }
    
    if ((transfer.is_outgoing || transfer.accepted) && !should_allow_auto_accept) {
        qCDebug(lcChatWindow) << "Skip auto-accept; already outbound or accepted" << message_id;
        return;
    }
    if (transfer.packet_no == 0 || transfer.file_id == 0) {
        qCDebug(lcChatWindow) << "Skip auto-accept; missing identifiers" << message_id
                              << transfer.packet_no
                              << transfer.file_id;
        return;
    }

    QString content_type = transfer.content_type;
    if (content_type.isEmpty() && !transfer.filename.isEmpty()) {
        content_type = imageMimeType(transfer.filename);
    }
    if (!content_type.startsWith(QStringLiteral("image/"))) {
        qCDebug(lcChatWindow) << "Skip auto-accept; content type not image" << message_id
                              << content_type;
        return;
    }

    QDir target_dir(m_auto_image_download_dir);
    if (!target_dir.exists() && !target_dir.mkpath(QStringLiteral("."))) {
        appendSystemMessage(QStringLiteral("自动保存图片失败，无法访问目录：%1")
                                .arg(QDir::toNativeSeparators(m_auto_image_download_dir).toHtmlEscaped()));
        qCWarning(lcChatWindow) << "Failed to access auto-download directory"
                                << m_auto_image_download_dir;
        return;
    }

    QString filename = transfer.filename;
    if (filename.isEmpty()) {
        QString suffix;
        const int slash = content_type.indexOf('/');
        if (slash >= 0) {
            suffix = content_type.mid(slash + 1);
        }
        filename = QStringLiteral("image_%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
        if (!suffix.isEmpty()) {
            filename += QLatin1Char('.');
            filename += suffix;
        }
    }

    const QString save_path = makeUniqueFilePath(target_dir.absolutePath(), filename);

    if (!m_network_manager->acceptFile(transfer.peer_id,
                                       transfer.packet_no,
                                       transfer.file_id,
                                       save_path)) {
        appendSystemMessage(QStringLiteral("自动接收图片失败：%1")
                                .arg(QDir::toNativeSeparators(save_path).toHtmlEscaped()));
        qCWarning(lcChatWindow) << "acceptFile failed for auto-download"
                                << transfer.peer_id
                                << transfer.packet_no
                                << transfer.file_id
                                << save_path;
        return;
    }

    transfer.accepted = true;
    transfer.saved_path = save_path;
    m_pending_transfers.insert(message_id, transfer);

    updateMessageMetadata(message_id,
                          {{QStringLiteral("transfer_status"), QStringLiteral("in_progress")},
                           {QStringLiteral("saved_path"), save_path}});

    appendSystemMessage(QStringLiteral("已自动接收图片，保存到 %1")
                            .arg(QDir::toNativeSeparators(save_path).toHtmlEscaped()));
    qCInfo(lcChatWindow) << "Auto-accepted image transfer" << message_id
                         << transfer.filename
                         << save_path;
}

void ChatWindow::applyFileOfferToTransfer(const QString& message_id, const DeferredFileOffer& offer)
{
    if (message_id.isEmpty() || !m_pending_transfers.contains(message_id)) {
        qCDebug(lcChatWindow) << "Defer file offer; missing transfer" << message_id
                              << offer.packet_no << offer.file_id;
        return;
    }

    PendingTransfer transfer = m_pending_transfers.value(message_id);
    transfer.packet_no = offer.packet_no;
    transfer.file_id = offer.file_id;
    transfer.peer_id = offer.sender_id;
    transfer.peer_ip = offer.sender_ip;
    transfer.accepted = false;
    transfer.completed = false;
    if (offer.filesize > 0) {
        transfer.filesize = offer.filesize;
    }
    if (transfer.filename.isEmpty()) {
        transfer.filename = offer.filename;
    }
    
    // 修复：如果文件提供来自网络（不是本地发送的），应该标记为接收
    // 即使senderId是local_user_id（loopback场景），只要消息是通过网络接收的，就应该允许自动下载
    if (offer.sender_id == m_local_user_id && m_is_loopback) {
        // Loopback场景：如果消息是通过网络接收的（即从自己发送给自己），应该允许自动下载
        // 但需要检查消息是否真的是接收的（通过检查消息的sender_id和receiver_id）
        // 这里我们通过检查transfer是否已经标记为outgoing来判断
        // 如果transfer.is_outgoing为true，说明这是发送的消息，不应该自动下载
        // 但如果这是通过addReceivedMessage添加的，is_outgoing应该是false
        // 为了安全，我们检查消息历史中对应的消息
        bool is_received_message = false;
        for (const ChatMessage& msg : m_message_history) {
            if (msg.message_id == message_id) {
                // 如果消息的sender_id是local_user_id但receiver_id也是local_user_id（loopback）
                // 并且是通过addReceivedMessage添加的（is_sent=false），则应该允许自动下载
                is_received_message = (msg.sender_id == m_local_user_id && 
                                      msg.receiver_id == m_local_user_id);
                break;
            }
        }
        // 如果是接收的消息（loopback），重置is_outgoing为false以允许自动下载
        if (is_received_message && transfer.is_outgoing) {
            transfer.is_outgoing = false;
            qCDebug(lcChatWindow) << "Reset is_outgoing for loopback received message" << message_id;
        }
    } else if (offer.sender_id != m_local_user_id) {
        // 来自其他用户的消息，明确标记为接收
        transfer.is_outgoing = false;
    }
    
    m_pending_transfers.insert(message_id, transfer);

    updateMessageMetadata(message_id,
                         {{QStringLiteral("transfer_status"), QStringLiteral("offered")}});

    if (!transfer.is_outgoing) {
        const QString display_name = peerDisplayName();
        const QString shown_name = transfer.filename.isEmpty() ? offer.filename : transfer.filename;
        appendSystemMessage(QStringLiteral("收到来自 %1 的文件提供：%2")
                                .arg(display_name, shown_name));
        qCInfo(lcChatWindow) << "Applied file offer" << message_id
                             << shown_name
                             << offer.packet_no
                             << offer.file_id;
    }

    maybeAutoAcceptImageTransfer(message_id);
}

QString ChatWindow::renderEmojiHtml(const QString& text) const
{
    QString result = text;
    for (auto it = m_emoji_resource_map.constBegin(); it != m_emoji_resource_map.constEnd(); ++it) {
        if (!result.contains(it.key())) {
            continue;
        }
        result.replace(it.key(), emojiImageTag(it.key()));
    }
    return result;
}

QString ChatWindow::emojiImageTag(const QString& emoji) const
{
    const QString path = m_emoji_resource_map.value(emoji);
    if (path.isEmpty()) {
        return QStringLiteral("<span style='font-size: 24px;'>%1</span>").arg(emoji.toHtmlEscaped());
    }
    return QStringLiteral("<img src='%1' width='24' height='24' style='vertical-align:middle;' alt='%2' />")
        .arg(path, emoji.toHtmlEscaped());
}

QString ChatWindow::imageMimeType(const QString& filename) const
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

void ChatWindow::onMessageLinkActivated(const QUrl& url)
{
    if (url.scheme() == QStringLiteral("download")) {
        QString transfer_id = url.path();
        if (transfer_id.startsWith('/')) {
            transfer_id.remove(0, 1);
        }

        if (!m_pending_transfers.contains(transfer_id)) {
            appendSystemMessage(QStringLiteral("无法找到文件记录，可能已过期。"));
            return;
        }

        PendingTransfer transfer = m_pending_transfers.value(transfer_id);
        if (transfer.is_outgoing) {
            if (!transfer.local_path.isEmpty() && QFileInfo::exists(transfer.local_path)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(transfer.local_path));
            } else {
                appendSystemMessage(QStringLiteral("未找到本地文件副本，原路径：%1")
                                        .arg(transfer.local_path.isEmpty()
                                                 ? QStringLiteral("未知")
                                                 : QDir::toNativeSeparators(transfer.local_path)));
            }
            return;
        }

        if (!transfer.saved_path.isEmpty() && QFileInfo::exists(transfer.saved_path)) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(transfer.saved_path));
            return;
        }

        if (transfer.accepted) {
            appendSystemMessage(QStringLiteral("文件正在接收中，请稍候…"));
            return;
        }

        if (transfer.packet_no == 0 || transfer.file_id == 0) {
            appendSystemMessage(QStringLiteral("缺少文件传输标识，无法继续。"));
            return;
        }

        if (!m_network_manager) {
            appendSystemMessage(QStringLiteral("网络管理器不可用，无法请求文件。"));
            return;
        }

        const QString default_path = defaultDownloadPath(transfer.filename);
        const QString save_path = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("保存文件"),
            default_path);
        if (save_path.isEmpty()) {
            return;
        }

        if (!m_network_manager->acceptFile(transfer.peer_id,
                                           transfer.packet_no,
                                           transfer.file_id,
                                           save_path)) {
            appendSystemMessage(QStringLiteral("无法发起文件接收，请稍后重试。"));
            return;
        }

        transfer.accepted = true;
        transfer.saved_path = save_path;
        m_pending_transfers.insert(transfer_id, transfer);
        appendSystemMessage(
            QStringLiteral("已开始接收文件（保存到 %1）")
                .arg(QDir::toNativeSeparators(save_path)));

        updateMessageMetadata(transfer_id,
                              {{QStringLiteral("transfer_status"), QStringLiteral("in_progress")},
                               {QStringLiteral("saved_path"), QDir::toNativeSeparators(save_path)}});
        return;
    }

    if (url.scheme() == QStringLiteral("http") ||
        url.scheme() == QStringLiteral("https")) {
        QDesktopServices::openUrl(url);
    }
}

void ChatWindow::registerFileTransfer(const ChatMessage& message)
{
    PendingTransfer transfer = m_pending_transfers.value(message.message_id);
    transfer.message_id = message.message_id;
    transfer.filename = message.metadata.value(QStringLiteral("file_name"));
    if (transfer.filename.isEmpty()) {
        transfer.filename = message.content;
    }

    bool ok = false;
    transfer.filesize = message.metadata.value(QStringLiteral("size")).toULongLong(&ok);
    transfer.file_hash = message.metadata.value(QStringLiteral("hash"));

    bool okPacket = false;
    bool okFile = false;
    transfer.packet_no = message.metadata.value(QStringLiteral("packet_no")).toUInt(&okPacket);
    transfer.file_id = message.metadata.value(QStringLiteral("file_id")).toUInt(&okFile);
    transfer.peer_id = message.sender_id;
    transfer.peer_ip = message.metadata.value(QStringLiteral("sender_ip"));
    transfer.local_path = message.metadata.value(QStringLiteral("local_path"));
    transfer.saved_path = message.metadata.value(QStringLiteral("saved_path"));
    transfer.content_type = message.metadata.value(QStringLiteral("content_type"));
    const QString status = message.metadata.value(QStringLiteral("transfer_status"));
    transfer.completed = (status == QStringLiteral("completed"));
    transfer.accepted = (status == QStringLiteral("in_progress"));
    
    // 修复：更准确地判断是否为发送消息
    // 如果消息的sender_id是local_user_id，但这是通过addReceivedMessage添加的（即is_sent=false）
    // 说明这是loopback场景下的接收消息，应该允许自动下载
    // 我们通过检查消息是否在历史记录中，以及是否真的是发送的消息来判断
    // 如果transfer已经存在且is_outgoing为false，说明这是接收的消息，不应该被覆盖
    if (transfer.is_outgoing && message.sender_id == m_local_user_id && m_is_loopback) {
        // 在loopback场景下，如果消息的receiver_id也是local_user_id，可能是接收的消息
        // 但这里我们无法直接判断，所以保持原有逻辑
        // 真正的修复在applyFileOfferToTransfer中
    } else {
        transfer.is_outgoing = (message.sender_id == m_local_user_id);
    }
    
    if (transfer.is_outgoing) {
        transfer.peer_id = m_peer_info.user_id;
    }

    m_pending_transfers.insert(message.message_id, transfer);

    if (transfer.packet_no != 0 && transfer.file_id != 0) {
        const quint64 key = transferKey(transfer.packet_no, transfer.file_id);
        m_transfer_index.insert(key, message.message_id);

        if (m_pending_file_offers.contains(key)) {
            const DeferredFileOffer offer = m_pending_file_offers.take(key);
            applyFileOfferToTransfer(message.message_id, offer);
        }
    }
}

void ChatWindow::onNetworkFileOffer(const QString& senderId,
                                    const QString& senderIp,
                                    quint32 packetNo,
                                    quint32 fileId,
                                    const QString& filename,
                                    quint64 filesize)
{
    if (senderId != m_peer_info.user_id && senderId != m_local_user_id) {
        return;
    }

    const quint64 key = transferKey(packetNo, fileId);
    const QString message_id = m_transfer_index.value(key);
    DeferredFileOffer offer;
    offer.sender_id = senderId;
    offer.sender_ip = senderIp;
    offer.packet_no = packetNo;
    offer.file_id = fileId;
    offer.filename = filename;
    offer.filesize = filesize;

    if (message_id.isEmpty() || !m_pending_transfers.contains(message_id)) {
        m_pending_file_offers.insert(key, offer);
        return;
    }

    // 修复：如果文件提供来自网络（通过UDP/TCP接收），应该允许自动下载
    // 在loopback场景下，如果文件提供是通过网络接收的（即通过onNetworkFileOffer），
    // 说明这是接收的消息，应该允许自动下载，不管is_outgoing是什么
    PendingTransfer existing = m_pending_transfers.value(message_id);
    if (existing.is_outgoing && senderId == m_local_user_id && m_is_loopback) {
        // Loopback场景：文件提供来自网络，说明这是接收的消息
        // 检查消息历史，如果消息的receiver_id是local_user_id，说明这是接收的消息
        bool is_received = false;
        for (const ChatMessage& msg : m_message_history) {
            if (msg.message_id == message_id) {
                // 如果消息的receiver_id是local_user_id，说明这是接收的消息
                is_received = (msg.receiver_id == m_local_user_id);
                break;
            }
        }
        // 如果找不到消息历史，但文件提供来自网络，也应该允许（因为这是通过网络接收的）
        // 重置is_outgoing为false以允许自动下载
        existing.is_outgoing = false;
        m_pending_transfers.insert(message_id, existing);
        qCDebug(lcChatWindow) << "Reset is_outgoing for network-received file offer" << message_id
                              << "is_received:" << is_received;
    } else if (existing.is_outgoing && senderId != m_local_user_id) {
        // 来自其他用户的消息，明确标记为接收
        existing.is_outgoing = false;
        m_pending_transfers.insert(message_id, existing);
        qCDebug(lcChatWindow) << "Reset is_outgoing for received file offer from other user" << message_id;
    }

    applyFileOfferToTransfer(message_id, offer);
}

void ChatWindow::onNetworkFileTransferProgress(const QString& peerId,
                                               quint32 packetNo,
                                               quint32 fileId,
                                               quint64 bytesTransferred,
                                               quint64 totalBytes)
{
    if (peerId != m_peer_info.user_id && peerId != m_local_user_id) {
        return;
    }

    const quint64 key = transferKey(packetNo, fileId);
    const QString message_id = m_transfer_index.value(key);
    if (message_id.isEmpty() || !m_pending_transfers.contains(message_id)) {
        return;
    }

    PendingTransfer transfer = m_pending_transfers.value(message_id);
    transfer.bytes_transferred = bytesTransferred;
    if (totalBytes > 0) {
        transfer.filesize = totalBytes;
    }
    m_pending_transfers.insert(message_id, transfer);

    updateMessageMetadata(message_id,
                         {{QStringLiteral("transfer_status"), QStringLiteral("in_progress")},
                          {QStringLiteral("size"), totalBytes > 0 ? QString::number(totalBytes)
                                                                    : QString()}});
}

void ChatWindow::onNetworkFileTransferFinished(const QString& peerId,
                                               quint32 packetNo,
                                               quint32 fileId,
                                               const QString& savePath)
{
    if (peerId != m_peer_info.user_id && peerId != m_local_user_id) {
        return;
    }

    const quint64 key = transferKey(packetNo, fileId);
    const QString message_id = m_transfer_index.value(key);
    if (message_id.isEmpty() || !m_pending_transfers.contains(message_id)) {
        return;
    }

    PendingTransfer transfer = m_pending_transfers.value(message_id);
    transfer.completed = true;
    transfer.accepted = false;
    if (!savePath.isEmpty()) {
        transfer.saved_path = savePath;
    }
    if (transfer.filesize == 0 && transfer.bytes_transferred > 0) {
        transfer.filesize = transfer.bytes_transferred;
    }
    m_pending_transfers.insert(message_id, transfer);

    updateMessageMetadata(message_id,
                         {{QStringLiteral("transfer_status"), QStringLiteral("completed")},
                          {QStringLiteral("saved_path"), transfer.saved_path}});

    if (transfer.is_outgoing) {
        appendSystemMessage(QStringLiteral("文件发送完成：%1")
                                .arg(transfer.filename.toHtmlEscaped()));
    } else {
        const QString path = transfer.saved_path.isEmpty() ? savePath : transfer.saved_path;
        if (path.isEmpty()) {
            appendSystemMessage(QStringLiteral("已接收文件 \"%1\"。")
                                     .arg(transfer.filename.toHtmlEscaped()));
    } else {
        appendSystemMessage(
                QStringLiteral("文件 \"%1\" 已保存到 %2")
                    .arg(transfer.filename.toHtmlEscaped(),
                         QDir::toNativeSeparators(path).toHtmlEscaped()));
        }

        if (!transfer.saved_path.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(transfer.saved_path));
        }
    }
}

void ChatWindow::onNetworkFileTransferFailed(const QString& peerId,
                                             quint32 packetNo,
                                             quint32 fileId,
                                             const QString& reason)
{
    if (peerId != m_peer_info.user_id && peerId != m_local_user_id) {
        return;
    }

    const quint64 key = transferKey(packetNo, fileId);
    const QString message_id = m_transfer_index.value(key);
    if (message_id.isEmpty() || !m_pending_transfers.contains(message_id)) {
        return;
    }

    PendingTransfer transfer = m_pending_transfers.value(message_id);
    transfer.accepted = false;
    transfer.completed = false;
    m_pending_transfers.insert(message_id, transfer);

    const QString err = reason.isEmpty() ? QStringLiteral("未知错误") : reason;
    appendSystemMessage(QStringLiteral("文件 \"%1\" 传输失败：%2")
                            .arg(transfer.filename.toHtmlEscaped(), err.toHtmlEscaped()));

    updateMessageMetadata(message_id,
                         {{QStringLiteral("transfer_status"), QStringLiteral("failed")}});
}

quint64 ChatWindow::transferKey(quint32 packetNo, quint32 fileId) const
{
    return (static_cast<quint64>(packetNo) << 32) | static_cast<quint64>(fileId);
}

QString ChatWindow::defaultDownloadPath(const QString& filename) const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (base.isEmpty()) {
        base = QDir::homePath();
    }
    if (base.isEmpty()) {
        base = QDir::tempPath();
    }
    QDir dir(base);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return dir.filePath(filename.isEmpty() ? QStringLiteral("download.bin") : filename);
}

bool ChatWindow::isOutgoingMessage(const ChatMessage& message) const
{
    if (!m_local_user_id.isEmpty() && message.sender_id == m_local_user_id) {
        return true;
    }
    if (m_is_ai_chat && message.sender_id == QStringLiteral("user")) {
        return true;
    }
    if (m_is_loopback && message.sender_id == m_peer_info.user_id &&
        message.receiver_id == m_local_user_id) {
        return true;
    }
    return false;
}

void ChatWindow::rebuildMessageDisplay()
{
    if (!m_message_display) {
        return;
    }

    const QSignalBlocker blocker(m_message_display);
    m_message_display->clear();

    const QHash<QString, PendingTransfer> previousTransfers = m_pending_transfers;
    m_pending_transfers.clear();
    m_transfer_index.clear();
    m_pending_file_offers.clear();

    for (const ChatMessage& msg : m_message_history) {
        const bool is_sent = isOutgoingMessage(msg);
        addMessageToDisplay(msg, is_sent);

        if (msg.message_type == MessageContentType::File &&
            previousTransfers.contains(msg.message_id) &&
            m_pending_transfers.contains(msg.message_id)) {
            PendingTransfer restored = m_pending_transfers.value(msg.message_id);
            const PendingTransfer cached = previousTransfers.value(msg.message_id);

            if (restored.bytes_transferred == 0 && cached.bytes_transferred > 0) {
                restored.bytes_transferred = cached.bytes_transferred;
            }
            if (restored.local_path.isEmpty() && !cached.local_path.isEmpty()) {
                restored.local_path = cached.local_path;
            }
            if (restored.saved_path.isEmpty() && !cached.saved_path.isEmpty()) {
                restored.saved_path = cached.saved_path;
            }
            if (!cached.peer_ip.isEmpty() && restored.peer_ip.isEmpty()) {
                restored.peer_ip = cached.peer_ip;
            }
            if (cached.accepted) {
                restored.accepted = true;
            }
            if (cached.completed) {
                restored.completed = true;
            }

            m_pending_transfers.insert(msg.message_id, restored);
        }
    }

    if (m_auto_image_download_enabled) {
        for (auto it = m_pending_transfers.constBegin(); it != m_pending_transfers.constEnd(); ++it) {
            const PendingTransfer& transfer = it.value();
            if (transfer.is_outgoing || transfer.accepted) {
                continue;
            }
            maybeAutoAcceptImageTransfer(it.key());
        }
    }
}

void ChatWindow::updateMessageMetadata(const QString& message_id,
                                       const QMap<QString, QString>& changes)
{
    if (message_id.isEmpty() || changes.isEmpty()) {
        return;
    }

    const QString conv_key = conversationKey();

    const auto needsRedisplay = [](const QString& key) {
        return key == QStringLiteral("saved_path") ||
               key == QStringLiteral("local_path") ||
               key == QStringLiteral("data") ||
               key == QStringLiteral("content_type") ||
               key == QStringLiteral("file_name") ||
               key == QStringLiteral("size") ||
               key == QStringLiteral("transfer_status");
    };

    bool metadataChanged = false;
    bool requiresRedisplay = false;

    const auto applyChanges = [&](ChatMessage& msg) {
        for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
            const QString& key = it.key();
            const QString& value = it.value();
            const bool removed = value.isNull();

            if (removed) {
                if (msg.metadata.remove(key) > 0) {
                    metadataChanged = true;
                    if (needsRedisplay(key)) {
                        requiresRedisplay = true;
                    }
                }
            } else {
                if (msg.metadata.value(key) != value) {
                    metadataChanged = true;
                    if (needsRedisplay(key)) {
                        requiresRedisplay = true;
                    }
                }
                msg.metadata.insert(key, value);
            }

            applyPendingTransferMetadata(message_id, key, value, removed);
        }
    };

    bool updated = false;
    for (ChatMessage& msg : m_message_history) {
        if (msg.message_id != message_id) {
            continue;
        }

        applyChanges(msg);
        if (m_message_repository && !conv_key.isEmpty()) {
            m_message_repository->saveMessage(msg, conv_key);
        }
        updated = true;
        break;
    }

    if (!updated && m_message_repository && !conv_key.isEmpty()) {
        ChatMessage msg;
        if (m_message_repository->loadMessage(message_id, conv_key, msg)) {
            applyChanges(msg);
            m_message_repository->saveMessage(msg, conv_key);
        }
    }

    if (metadataChanged && requiresRedisplay) {
        rebuildMessageDisplay();
    }
}

void ChatWindow::persistMessage(const ChatMessage& message)
{
    if (!m_message_repository) {
        return;
    }
    const QString key = conversationKey();
    if (key.isEmpty()) {
        return;
    }
    m_message_repository->saveMessage(message, key);
}

QString ChatWindow::conversationKey() const
{
    QStringList participants;
    if (m_network_manager) {
        participants.append(m_network_manager->getLocalUserId());
    }
    participants.append(m_peer_info.user_id);
    participants.removeAll(QString());
    if (participants.isEmpty()) {
        return QString();
    }
    std::sort(participants.begin(), participants.end());
    return participants.join(QLatin1Char(':'));
}

QString ChatWindow::formatFileSize(quint64 bytes) const
{
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    int unit_index = 0;
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        ++unit_index;
    }

    if (unit_index == 0) {
        return QString::number(static_cast<qulonglong>(size)) + " " + units[unit_index];
    }
    return QString::number(size, 'f', 1) + " " + units[unit_index];
}

bool ChatWindow::evaluateCompliance(const CompliancePayload& payload, ComplianceResult& result) const
{
    if (!m_compliance_service) {
        result = {ComplianceVerdict::Allowed, QString()};
        return true;
    }

    if (!m_compliance_service->isAvailable()) {
        result = {ComplianceVerdict::Allowed, QString()};
        return true;
    }

    result = m_compliance_service->evaluate(payload);
    if (result.verdict == ComplianceVerdict::Blocked) {
        return false;
    }
    return true;
}

// ============================================================================
// 事件处理
// ============================================================================

void ChatWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        updateWindowTitle();
    }
    QWidget::changeEvent(event);

    // 当窗口激活时，通知主窗口（用于清除未读等）
    if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            emit chatActivated(m_peer_info.user_id);
        }
    }
}

void ChatWindow::closeEvent(QCloseEvent* event)
{
    // 通知主窗口聊天窗口已关闭
    emit chatClosed(m_peer_info.user_id);
    QWidget::closeEvent(event);
}

void ChatWindow::updateWindowTitle()
{
    if (isGroupChat()) {
        setWindowTitle(QStringLiteral("群组 - %1").arg(m_group_info.group_name));
        if (m_peer_name_label) {
            QString label = m_group_info.group_name;
            if (m_group_info.memberCount() > 0) {
                label += QStringLiteral(" (%1人)").arg(m_group_info.memberCount());
            }
            m_peer_name_label->setText(label);
            m_peer_name_label->setToolTip(QStringLiteral("群组名: %1\n群组ID: %2\n成员数: %3")
                                              .arg(m_group_info.group_name.isEmpty() ? QStringLiteral("<未知>") : m_group_info.group_name,
                                                   m_group_info.group_id.isEmpty() ? QStringLiteral("<未知>") : m_group_info.group_id,
                                                   QString::number(m_group_info.memberCount())));
        }
    } else {
        const QString display_name = peerDisplayName();
        setWindowTitle(QStringLiteral("聊天 - %1").arg(display_name));
        if (m_peer_name_label) {
            QString label = display_name;
            if (!m_peer_info.ip_address.isEmpty() && m_peer_info.ip_address != display_name) {
                label += QStringLiteral(" [%1]").arg(m_peer_info.ip_address);
            }
            m_peer_name_label->setText(label);
            m_peer_name_label->setToolTip(QStringLiteral("用户名: %1\n用户ID: %2\nIP: %3")
                                              .arg(m_peer_info.username.isEmpty() ? QStringLiteral("<未知>") : m_peer_info.username,
                                                   m_peer_info.user_id.isEmpty() ? QStringLiteral("<未知>") : m_peer_info.user_id,
                                                   m_peer_info.ip_address.isEmpty() ? QStringLiteral("<未知>") : m_peer_info.ip_address));
        }
    }
}

void ChatWindow::updatePresenceIcon()
{
    if (m_presence_icon) {
        m_presence_icon->setPixmap(statusIcon().pixmap(m_presence_icon->size()));
    }
}

void ChatWindow::updatePresenceTooltip()
{
    if (m_toolbar) {
        QString tooltip = statusDisplayText(m_peer_info.status);
        if (!m_peer_info.status_text.isEmpty()) {
            tooltip += QStringLiteral(" · %1").arg(m_peer_info.status_text);
        }
        m_toolbar->setToolTip(tooltip);
    }
}

QString ChatWindow::statusDisplayText(UserStatus status) const
{
    switch (status) {
    case UserStatus::Online:
        return QStringLiteral("在线");
    case UserStatus::Away:
        return QStringLiteral("离开");
    case UserStatus::Busy:
        return QStringLiteral("忙碌");
    case UserStatus::Invisible:
        return QStringLiteral("隐身");
    case UserStatus::Offline:
    default:
        return QStringLiteral("离线");
    }
}

QString ChatWindow::peerDisplayName() const
{
    const QString username = m_peer_info.username.trimmed();
    if (!username.isEmpty()) {
        return username;
    }

    const QString ip = m_peer_info.ip_address.trimmed();
    if (!ip.isEmpty()) {
        return ip;
    }

    const QString user_id = m_peer_info.user_id.trimmed();
    if (!user_id.isEmpty()) {
        return user_id;
    }

    return QStringLiteral("未知用户");
}

QIcon ChatWindow::actionIcon(const QString& name) const
{
    const QString path = QStringLiteral(":/icons/%1.svg").arg(name);
    QIcon icon(path);
    if (!icon.isNull()) {
        return icon;
    }
    return style()->standardIcon(QStyle::SP_FileIcon);
}

QIcon ChatWindow::statusIcon() const
{
    switch (m_peer_info.status) {
    case UserStatus::Online:
        return QIcon(":/icons/status_online.svg");
    case UserStatus::Away:
        return QIcon(":/icons/status_away.svg");
    case UserStatus::Busy:
        return QIcon(":/icons/status_busy.svg");
    case UserStatus::Invisible:
        return QIcon(":/icons/status_invisible.svg");
    default:
        return QIcon(":/icons/status_offline.svg");
    }
}

} // namespace KylinMessenger

#include "chat_window.moc"
