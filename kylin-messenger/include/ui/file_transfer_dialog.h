#ifndef KYLIN_MESSENGER_UI_FILE_TRANSFER_DIALOG_H
#define KYLIN_MESSENGER_UI_FILE_TRANSFER_DIALOG_H

#include <QDialog>
#include <QFileInfo>

class QLineEdit;
class QLabel;
class QPushButton;
class QRadioButton;

namespace KylinMessenger {

class FileTransferDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Protocol {
        UDP,
        TCP
    };

    explicit FileTransferDialog(QWidget* parent = nullptr);

    QFileInfo selectedFile() const { return m_file_info; }
    Protocol selectedProtocol() const;
    bool isImage() const { return m_is_image; }
    qint64 fileSize() const { return m_file_size; }

private slots:
    void onBrowseButtonClicked();
    void onPathTextEdited(const QString& text);
    void onProtocolToggled();

private:
    void updateFileState(const QString& path);
    void updateHint();
    void updateButtonState();
    bool loadImagePreview(const QFileInfo& info);
    static QString formatSize(qint64 bytes);

    QLineEdit* m_path_edit;
    QLabel* m_info_label;
    QLabel* m_preview_label;
    QLabel* m_hint_label;
    QPushButton* m_browse_button;
    QRadioButton* m_udp_radio;
    QRadioButton* m_tcp_radio;
    QPushButton* m_ok_button;

    QFileInfo m_file_info;
    bool m_is_image = false;
    qint64 m_file_size = 0;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_UI_FILE_TRANSFER_DIALOG_H
