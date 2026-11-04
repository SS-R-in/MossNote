#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTreeWidgetItem>
#include <QFile>    // 打开和读取文件
#include <QFileDialog>  // 让用户在弹出的窗口里选择文件
#include <QMessageBox>   // 读取失败时显示提示
#include <QSaveFile>
#include <QString>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QTreeWidgetItem>

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

    connect(ui->actionChooseVault, &QAction::triggered,
            this, &MainWindow::chooseVault);

    connect(ui->actionRefreshVault, &QAction::triggered,
            this, &MainWindow::refreshVault);

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
    Q_UNUSED(column);

    if (!maybeSave()) {
        return;
    }

    QString filePath = item->data(0, Qt::UserRole).toString();

    // 没有真实文件路径时，说明这是旧的示例条目
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "打开失败", file.errorString());
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    ui->editor->setPlainText(content);
    currentFilePath = filePath;
    ui->editor->document()->setModified(false);
}

// 让用户选择一个 Markdown 文件，读取内容，再显示到编辑器
void MainWindow::openMarkdownFile()
{
    if (!maybeSave()) {
        return;
    }

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
    currentFilePath = filePath; //成功打开并读取文件之后，才会更新当前路径。用户取消或打开失败时，原来的路径保持不变。
    ui->editor->document()->setModified(false);
}

void MainWindow::saveMarkdownFile()
{
    QString filePath = currentFilePath;

    if(filePath.isEmpty()) {
        filePath = QFileDialog::getSaveFileName(
            this,
            "保存 Markdown 文件",
            QString(),
            "Markdown 文件 (*.md)"
            );
    }

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
    // 保存成功后，记住这次保存的路径
    currentFilePath = filePath;
    ui->editor->document()->setModified(false);
    QMessageBox::information(this, "保存成功", "笔记已保存。");
}

bool MainWindow::maybeSave()
{
    if (!ui->editor->document()->isModified()) {
        return true;
    }

    QMessageBox::StandardButton answer =
        QMessageBox::warning(
            this,
            "未保存的修改",
            "当前笔记有未保存的修改，要保存吗？",
            QMessageBox::Save |
                QMessageBox::Discard |
                QMessageBox::Cancel
            );

    if (answer == QMessageBox::Save) {
        saveMarkdownFile();

        // 如果用户在保存窗口中取消，仍然是修改状态
        return !ui->editor->document()->isModified();
    }

    if (answer == QMessageBox::Discard) {
        return true;
    }

    return false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();  // 允许关闭窗口
    } else {
        event->ignore();  // 取消关闭，留在当前窗口
    }
}

void MainWindow::chooseVault()
{
    if (!maybeSave()) {
        return;
    }

    QString folder = QFileDialog::getExistingDirectory( // 让用户选择文件夹
        this,
        "选择笔记库文件夹"
        );

    if (folder.isEmpty()) {
        return;
    }

    vaultPath = folder;
    currentFilePath.clear();
    ui->noteTree->clear();

    QDir dir(vaultPath);    // 扫描其中的 .md 文件
    const QStringList files = dir.entryList(
        QStringList{"*.md", "*.MD"},
        QDir::Files,
        QDir::Name
        );
    // 每个条目显示不带扩展名的文件名，同时把完整路径存到条目的 Qt::UserRole 数据里，点击时就能知道具体要打开哪个文件。
    for (const QString &fileName : files) {
        QString fullPath = dir.filePath(fileName);
        auto *item = new QTreeWidgetItem(ui->noteTree);
        item->setText(0, QFileInfo(fileName).completeBaseName());
        item->setData(0, Qt::UserRole, fullPath);
    }
}

void MainWindow::refreshVault()
{
    if (vaultPath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择笔记库文件夹。");
        return;
    }

    ui->noteTree->clear();

    QDir dir(vaultPath);
    const QStringList files = dir.entryList(
        QStringList{"*.md", "*.MD"},
        QDir::Files,
        QDir::Name
        );

    for (const QString &fileName : files) {
        const QString fullPath = dir.filePath(fileName);
        auto *item = new QTreeWidgetItem(ui->noteTree);
        item->setText(0, QFileInfo(fileName).completeBaseName());
        item->setData(0, Qt::UserRole, fullPath);
    }
}