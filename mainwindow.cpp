#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTreeWidgetItem>
#include <QFile>    // 打开和读取文件
#include <QFileDialog>  // 让用户在弹出的窗口里选择文件
#include <QMessageBox>   // 读取失败时显示提示
#include <QSaveFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 隐藏左侧默认显示“1”的表头
    ui->noteTree->setHeaderHidden(true);

    // 添加两条示例笔记
    new QTreeWidgetItem(ui->noteTree, QStringList{"学习 C++"});
    new QTreeWidgetItem(ui->noteTree, QStringList{"学习 Qt"});

    // editor内容发生变化时发出信号，调用updatePreview
    connect(ui->editor, &QPlainTextEdit::textChanged,
            this, &MainWindow::updatePreview);

    // 左侧某个条目被点击时发信号，调用openSampleNote
    connect(ui->noteTree, &QTreeWidget::itemClicked,
            this, &MainWindow::openSampleNote);

    // 用户点击“打开文件”菜单项时，让程序调用 openMarkdownFile()
    connect(ui->actionOpen, &QAction::triggered,    // 触发
            this, &MainWindow::openMarkdownFile);

    connect(ui->actionSave, &QAction::triggered,
            this, &MainWindow::saveMarkdownFile);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 把文本转换成markdown形式
void MainWindow::updatePreview()
{
    ui->preview->setMarkdown(ui->editor->toPlainText());
}

void MainWindow::openSampleNote(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column); // 目前不需要用到列号

    if (item->text(0) == "学习 C++") {    // 读取被点击条目第 0 列的文字
        ui->editor->setPlainText("# 学习 C++\n\n- 复习类和对象");   // 示例内容放进编辑器
    } else if (item->text(0) == "学习 Qt") {
        ui->editor->setPlainText("# 学习 Qt\n\n- 理解信号与槽");
    }
}

// 让用户选择一个 Markdown 文件，读取内容，再显示到编辑器
void MainWindow::openMarkdownFile()
{
    // 让用户选择一个 Markdown 文件
    QString filePath = QFileDialog::getOpenFileName(    // 弹出系统文件选择窗口，并把用户选择的路径保存到 filePath
        this,   // 表示这个选择窗口属于当前主窗口
        "打开 Markdown 文件",   // 文件选择窗口的标题
        QString(),  // 表示初始打开目录为空，系统会使用默认位置。以后也可以写成某个目录路径。
        "Markdown 文件 (*.md);;所有文件 (*)"  // 文件类型过滤器
        );

    // 用户点了取消，直接返回
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);   // 创建一个 QFile 对象，让它代表用户选中的文件。

    // 以只读方式打开；失败时提示原因
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "打开失败", file.errorString());
        return;
    }

    // 文件按 UTF-8 解码，再放进编辑器
    QString content = QString::fromUtf8(file.readAll());
    ui->editor->setPlainText(content);
}

void MainWindow::saveMarkdownFile()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "保存 Markdown 文件",
        QString(),
        "Markdown 文件 (*.md)"
        );

    // 用户取消保存
    if (filePath.isEmpty()) {
        return;
    }

    // 用户没有写扩展名时，补上 .md
    if (!filePath.endsWith(".md", Qt::CaseInsensitive)) {
        filePath += ".md";
    }

    QSaveFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "保存失败", file.errorString());
        return;
    }

    QByteArray content = ui->editor->toPlainText().toUtf8();

    if (file.write(content) != content.size()) {
        QMessageBox::warning(this, "保存失败", file.errorString());
        return;
    }

    if (!file.commit()) {
        QMessageBox::warning(this, "保存失败", file.errorString());
        return;
    }

    QMessageBox::information(this, "保存成功", "笔记已保存。");
}