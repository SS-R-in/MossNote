#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE

// 让头文件认识 QTreeWidgetItem
class QTreeWidgetItem;

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
    void saveMarkdownFile();

private:
    Ui::MainWindow *ui;
    QString currentFilePath;    // 默认是空字符串，表示目前没有关联的文件
};
#endif // MAINWINDOW_H
