#include "ui/file_transfer_dialog.h"

#include <QBoxLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QPixmap>
#include <QSize>
#include <QStringList>

namespace KylinMessenger {

namespace {
constexpr qint64 kUdpPayloadSafeBytes = 48 * 1024; // base64 约膨胀 4/3, 48KB -> ~64KB
constexpr int kPreviewSize = 280;
}

FileTransferDialog::FileTransferDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("发送文件"));
    setModal(true);
    setMinimumWidth(480);

    m_path_edit = new QLineEdit(this);
    m_path_edit->setPlaceholderText(QStringLiteral("选择要发送的文件"));
    m_path_edit->setClearButtonEnabled(true);

    m_browse_button = new QPushButton(QStringLiteral("浏览..."), this);

    QHBoxLayout* path_layout = new QHBoxLayout;
    path_layout->addWidget(m_path_edit, 1);
    path_layout->addWidget(m_browse_button);

    m_udp_radio = new QRadioButton(QStringLiteral("UDP (快速发送，适合小图片)"), this);
    m_tcp_radio = new QRadioButton(QStringLiteral("TCP (可靠传输，适合大文件)"), this);
    m_tcp_radio->setChecked(true);

    QVBoxLayout* protocol_layout = new QVBoxLayout;
    protocol_layout->addWidget(m_udp_radio);
    protocol_layout->addWidget(m_tcp_radio);

    m_info_label = new QLabel(QStringLiteral("未选择文件"), this);
    m_hint_label = new QLabel(QString(), this);
    m_hint_label->setWordWrap(true);
    m_hint_label->setStyleSheet(QStringLiteral("color:#666;"));

    m_preview_label = new QLabel(this);
    m_preview_label->setFixedSize(kPreviewSize, kPreviewSize);
    m_preview_label->setAlignment(Qt::AlignCenter);
    m_preview_label->setStyleSheet(QStringLiteral("border:1px dashed #aaa; background-color:#fafafa;"));
    m_preview_label->setText(QStringLiteral("预览不可用"));

    QVBoxLayout* preview_layout = new QVBoxLayout;
    preview_layout->addWidget(new QLabel(QStringLiteral("预览"), this));
    preview_layout->addWidget(m_preview_label);

    QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_ok_button = button_box->button(QDialogButtonBox::Ok);
    m_ok_button->setEnabled(false);

    connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->addLayout(path_layout);
    main_layout->addLayout(protocol_layout);
    main_layout->addWidget(m_info_label);
    main_layout->addWidget(m_hint_label);
    main_layout->addLayout(preview_layout);
    main_layout->addStretch(1);
    main_layout->addWidget(button_box);

    connect(m_browse_button, &QPushButton::clicked, this, &FileTransferDialog::onBrowseButtonClicked);
    connect(m_path_edit, &QLineEdit::textChanged, this, &FileTransferDialog::onPathTextEdited);
    connect(m_udp_radio, &QRadioButton::toggled, this, &FileTransferDialog::onProtocolToggled);
    connect(m_tcp_radio, &QRadioButton::toggled, this, &FileTransferDialog::onProtocolToggled);

    updateHint();
}

FileTransferDialog::Protocol FileTransferDialog::selectedProtocol() const
{
    return m_udp_radio->isChecked() ? Protocol::UDP : Protocol::TCP;
}

void FileTransferDialog::onBrowseButtonClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择要发送的文件"));
    if (!path.isEmpty()) {
        m_path_edit->setText(path);
    }
}

void FileTransferDialog::onPathTextEdited(const QString& text)
{
    updateFileState(text.trimmed());
}

void FileTransferDialog::onProtocolToggled()
{
    updateHint();
    updateButtonState();
}

void FileTransferDialog::updateFileState(const QString& path)
{
    m_file_info = QFileInfo();
    m_is_image = false;
    m_file_size = 0;

    if (path.isEmpty()) {
        m_info_label->setText(QStringLiteral("未选择文件"));
        m_preview_label->setPixmap(QPixmap());
        m_preview_label->setText(QStringLiteral("预览不可用"));
        updateHint();
        updateButtonState();
        return;
    }

    QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        m_info_label->setText(QStringLiteral("文件不存在或不可访问"));
        m_preview_label->setPixmap(QPixmap());
        m_preview_label->setText(QStringLiteral("预览不可用"));
        updateHint();
        updateButtonState();
        return;
    }

    m_file_info = info;
    m_file_size = info.size();

    const QString base = info.fileName();
    const QString size_text = formatSize(m_file_size);
    m_info_label->setText(QStringLiteral("已选择：%1 (%2)").arg(base, size_text));

    m_is_image = loadImagePreview(info);

    if (!m_is_image) {
        QFileIconProvider icon_provider;
        QPixmap icon = icon_provider.icon(info).pixmap(64, 64);
        if (!icon.isNull()) {
            m_preview_label->setPixmap(icon.scaled(kPreviewSize / 2, kPreviewSize / 2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_preview_label->setText(QString());
        } else {
            m_preview_label->setPixmap(QPixmap());
            m_preview_label->setText(QStringLiteral("预览不可用"));
        }
    }

    // 自动推荐协议
    if (!m_is_image || m_file_size > kUdpPayloadSafeBytes) {
        m_tcp_radio->setChecked(true);
    } else if (!m_tcp_radio->isChecked() && !m_udp_radio->isChecked()) {
        m_udp_radio->setChecked(true);
    }

    updateHint();
    updateButtonState();
}

void FileTransferDialog::updateHint()
{
    if (!m_file_info.exists()) {
        m_hint_label->setText(QStringLiteral("请选择要发送的文件。"));
        return;
    }

    const bool udpSelected = (selectedProtocol() == Protocol::UDP);
    QStringList hints;

    if (udpSelected) {
        if (!m_is_image) {
            hints << QStringLiteral("UDP 仅适合发送小图片，当前文件将回退为 TCP 传输。");
        } else if (m_file_size > kUdpPayloadSafeBytes) {
            hints << QStringLiteral("图片较大，建议改用 TCP 以确保可靠性。");
        } else {
            hints << QStringLiteral("将以 UDP 发送图片，速度更快但若丢包不会重传。");
        }
    } else {
        hints << QStringLiteral("TCP 会建立连接并可靠传输，适合大文件和文档。");
    }

    hints << QStringLiteral("文件大小：%1").arg(formatSize(m_file_size));
    if (m_is_image) {
        hints << QStringLiteral("图片格式：%1").arg(m_file_info.suffix().toUpper());
    }

    m_hint_label->setText(hints.join(QStringLiteral("\n")));
}

void FileTransferDialog::updateButtonState()
{
    const bool valid = m_file_info.exists() && m_file_info.isFile();
    if (!valid) {
        m_ok_button->setEnabled(false);
        return;
    }

    if (selectedProtocol() == Protocol::UDP) {
        const bool allow_udp = m_is_image && (m_file_size > 0 && m_file_size <= kUdpPayloadSafeBytes);
        m_ok_button->setEnabled(allow_udp);
        if (!allow_udp) {
            m_hint_label->setText(QStringLiteral("当前选择的文件无法通过 UDP 发送，请切换到 TCP。"));
        }
    } else {
        m_ok_button->setEnabled(true);
    }
}

bool FileTransferDialog::loadImagePreview(const QFileInfo& info)
{
    QImageReader reader(info.absoluteFilePath());
    if (!reader.canRead()) {
        m_preview_label->setPixmap(QPixmap());
        m_preview_label->setText(QStringLiteral("预览不可用"));
        return false;
    }

    reader.setAutoTransform(true);
    reader.setScaledSize(QSize(kPreviewSize, kPreviewSize));
    const QImage image = reader.read();
    if (image.isNull()) {
        m_preview_label->setPixmap(QPixmap());
        m_preview_label->setText(QStringLiteral("预览不可用"));
        return false;
    }

    QPixmap pixmap = QPixmap::fromImage(image.scaled(kPreviewSize, kPreviewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_preview_label->setPixmap(pixmap);
    m_preview_label->setText(QString());
    return true;
}

QString FileTransferDialog::formatSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    double size = static_cast<double>(bytes) / 1024.0;
    static const char* units[] = {"KB", "MB", "GB", "TB"};
    int unit_index = 0;
    const int max_index = static_cast<int>(sizeof(units) / sizeof(units[0])) - 1;
    while (size >= 1024.0 && unit_index < max_index) {
        size /= 1024.0;
        ++unit_index;
    }
    return QStringLiteral("%1 %2").arg(QString::number(size, 'f', size < 10.0 ? 2 : 1), QLatin1String(units[unit_index]));
}

} // namespace KylinMessenger
