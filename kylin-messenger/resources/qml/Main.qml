import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: window
    width: 1080
    height: 680
    visible: true
    title: qsTr("Kylin Messenger · Quick Preview")
    property var host: appHost
    property string selectedUserId: ""
    property string selectedUserName: ""

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 12
            Label {
                text: host ? qsTr("当前用户：%1").arg(host.localUserName || qsTr("未知")) : qsTr("未连接")
                font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("刷新在线")
                enabled: host !== null
                onClicked: host && host.refreshOnlineUsers()
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Frame {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            padding: 0
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Label {
                    text: qsTr("在线联系人")
                    font.pixelSize: 16
                    padding: 12
                }
                ListView {
                    id: userList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: host ? host.userListModel : null
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: username
                        highlighted: ListView.isCurrentItem
                        onClicked: {
                            userList.currentIndex = index
                            window.selectedUserId = userId
                            window.selectedUserName = username
                        }
                        contentItem: Column {
                            spacing: 2
                            Text { text: username; font.bold: true }
                            Text {
                                text: statusText && statusText.length > 0 ? statusText : qsTr("IP: %1").arg(ip)
                                color: "#666666"
                                font.pixelSize: 12
                            }
                        }
                    }
                    footer: Label {
                        visible: (model ? model.count : 0) === 0
                        text: qsTr("暂无在线联系人")
                        color: "#888"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        height: 80
                    }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                spacing: 12
                Label {
                    text: selectedUserName.length > 0 ? qsTr("与 %1 对话").arg(selectedUserName) : qsTr("选择联系人开始聊天")
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }
                ListView {
                    id: messageFeed
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8
                    clip: true
                    model: host ? host.messageListModel : null
                    delegate: Item {
                        width: ListView.view.width
                        implicitHeight: bubble.implicitHeight + 8
                        Column {
                            id: bubble
                            width: parent.width
                            spacing: 2
                            anchors.horizontalCenter: outgoing ? undefined : undefined
                            Text {
                                text: qsTr("[%1] %2").arg(timestamp, senderName)
                                color: "#666"
                                font.pixelSize: 11
                                horizontalAlignment: outgoing ? Text.AlignRight : Text.AlignLeft
                                width: parent.width
                            }
                            Rectangle {
                                width: parent.width * 0.75
                                color: outgoing ? "#DCF8C6" : "#FFFFFF"
                                radius: 8
                                border.color: "#e0e0e0"
                                border.width: 1
                                anchors.left: outgoing ? undefined : parent.left
                                anchors.right: outgoing ? parent.right : undefined
                                Text {
                                    text: content
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    wrapMode: Text.Wrap
                                    color: "#222"
                                }
                            }
                        }
                    }
                    onCountChanged: positionViewAtEnd()
                    Component.onCompleted: positionViewAtEnd()
                }
                TextArea {
                    id: messageInput
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    wrapMode: TextArea.Wrap
                    placeholderText: qsTr("输入消息...")
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Button {
                        text: qsTr("广播消息")
                        enabled: host !== null && messageInput.text.trim().length > 0
                        onClicked: {
                            if (host && host.broadcastTextMessage(messageInput.text)) {
                                messageInput.clear()
                            }
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("发送")
                        enabled: host !== null && selectedUserId.length > 0 && messageInput.text.trim().length > 0
                        onClicked: {
                            if (host && host.sendTextMessage(selectedUserId, messageInput.text)) {
                                messageInput.clear()
                            }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: toastPopup
        x: (window.width - width) / 2
        y: window.height - height - 32
        padding: 12
        background: Rectangle {
            color: "#323232"
            radius: 12
            border.width: 0
        }
        contentItem: Label {
            text: toastPopup.text
            color: "white"
            font.pixelSize: 14
        }
        property string text: ""
    }

    Timer {
        id: toastTimer
        interval: 2400
        repeat: false
        onTriggered: toastPopup.close()
    }

    function showToast(message) {
        toastPopup.text = message
        toastPopup.open()
        toastTimer.restart()
    }

    Connections {
        target: host
        enabled: host !== null
        onToastRequested: window.showToast(message)
        onLocalUserChanged: {
            if (!host || host.localUserName.length === 0) {
                window.selectedUserId = ""
                window.selectedUserName = ""
            }
        }
    }
}
