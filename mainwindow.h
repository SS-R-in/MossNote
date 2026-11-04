#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE

// 让头文件认识 QTreeWidgetItem
class QTreeWidgetItem;
class QCloseEvent;

namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void updatePreview();
    void openSampleNote(QTreeWidgetItem *item, int column);
    void openMarkdownFile();
    void saveMarkdownFile();    // 检查当前内容是否修改过，并询问用户是否保存
    void chooseVault();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;
    QString currentFilePath;    // 默认是空字符串，表示目前没有关联的文件
    bool maybeSave();
    QString vaultPath;  // 记住用户选中的笔记文件夹路径
};
#endif // MAINWINDOW_H
