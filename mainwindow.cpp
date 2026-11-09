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
#include <QInputDialog> // 获取标题
#include <QLineEdit>
#include <QRegularExpression>
#include <QDirIterator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 隐藏左侧默认显示“1”的表头
    ui->noteTree->setHeaderHidden(true);

    // 添加两条示例笔记
    // new QTreeWidgetItem(ui->noteTree, QStringList{"学习 C++"});
    // new QTreeWidgetItem(ui->noteTree, QStringList{"学习 Qt"});

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

    connect(ui->actionNewNote, &QAction::triggered,
            this, &MainWindow::newNote);

    connect(ui->actionRenameNote, &QAction::triggered, this, &MainWindow::renameNote);

    connect(ui->actionDeleteNote, &QAction::triggered, this, &MainWindow::deleteNote);

    connect(ui->actionSearchNote, &QAction::triggered, this, &MainWindow::searchNote);

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

    QString filePath = item->data(0, Qt::UserRole).toString();

    // 文件夹没有存文件路径，点击它时不打开笔记
    if (filePath.isEmpty()) {
        return;
    }

    if (!maybeSave()) {
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
    refreshVault();
}

void MainWindow::refreshVault()
{
    if (vaultPath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择笔记库文件夹。");
        return;
    }

    ui->noteTree->clear();
    addNotesFromDirectory(vaultPath, nullptr);
}

void MainWindow::newNote()
{
    if (vaultPath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择笔记库文件夹。");
        return;
    }

    if (!maybeSave()) {
        return;
    }

    bool ok = false;
    QString title = QInputDialog::getText(
                        this,
                        "新建笔记",
                        "笔记名称：",
                        QLineEdit::Normal,
                        QString(),
                        &ok
                        ).trimmed();

    if (!ok || title.isEmpty()) {
        return;
    }

    // 避免把路径分隔符当成标题的一部分
    if (title.contains('/') || title.contains('\\')) {
        QMessageBox::warning(this, "名称无效", "笔记名称不能包含 / 或 \\。");
        return;
    }

    QString filePath = QDir(vaultPath).filePath(title + ".md");

    if (QFileInfo::exists(filePath)) {  // 检查文件名是否冲突
        QMessageBox::warning(this, "文件已存在", "笔记库里已经有这个文件名。");
        return;
    }

    QSaveFile file(filePath);   // 创建空 Markdown 文件
    if (!file.open(QIODevice::WriteOnly) || !file.commit()) {
        QMessageBox::warning(this, "创建失败", file.errorString());
        return;
    }

    // 创建后刷新左侧列表，并把新文件设为当前笔记。

    refreshVault();

    currentFilePath = filePath;
    ui->editor->clear();
    ui->editor->document()->setModified(false);
    ui->editor->setFocus();
}

void MainWindow::renameNote(){
    QTreeWidgetItem* item = ui->noteTree->currentItem();

    if(!item){
        QMessageBox::information(this, "提示", "请先在左侧选择一篇笔记。");
        return;
    }

    QString oldPath =item->data(0, Qt::UserRole).toString();

    // 文件夹没有存放笔记路径
    if (oldPath.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择笔记文件。");
        return;
    }

    const QFileInfo oldInfo(oldPath);
    const QString oldTitle = oldInfo.completeBaseName();


    bool ok = false;
    QString newTitle = QInputDialog::getText(
                        this,
                        "重命名笔记",
                        "笔记名称：",
                        QLineEdit::Normal,
                        QString(),
                        &ok
                        ).trimmed();

    if (!ok || newTitle.isEmpty() || oldTitle == newTitle) {
        return;
    }

    // 避免把路径分隔符当成标题的一部分
    if (newTitle.contains('/') || newTitle.contains('\\')) {
        QMessageBox::warning(this, "名称无效", "笔记名称不能包含 / 或 \\。");
        return;
    }

    QString newPath = oldInfo.dir().filePath(newTitle + ".md");

    if (QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, "文件已存在", "笔记库里已经有这个文件名。");
        return;
    }

    if (!QFile::rename(oldPath, newPath)) {
        QMessageBox::warning(this, "重命名失败", "无法重命名文件。");
        return;
    }

    // 如果改名的是正在编辑的文件，同步更新保存路径
    if (oldPath == currentFilePath) {
        currentFilePath = newPath;
    }

    refreshVault();
}

void MainWindow::addNotesFromDirectory(const QString &dirPath,
                                       QTreeWidgetItem *parentItem)
{
    QDir dir(dirPath);

    // 先列出当前目录下的子文件夹
    const QFileInfoList folders = dir.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks,   // 筛选条件
        QDir::Name | QDir::IgnoreCase   // 排序
        );

    for (const QFileInfo &folder : folders) {
        QTreeWidgetItem *folderItem;

        if (parentItem) {
            folderItem = new QTreeWidgetItem(parentItem);
        } else {
            folderItem = new QTreeWidgetItem(ui->noteTree);
        }

        folderItem->setText(0, folder.fileName());

        // 继续扫描这个子文件夹
        addNotesFromDirectory(folder.absoluteFilePath(), folderItem);
    }

    // 再列出当前目录下的 Markdown 文件
    const QFileInfoList files = dir.entryInfoList(
        QStringList{"*.md", "*.MD"},
        QDir::Files | QDir::NoSymLinks,
        QDir::Name | QDir::IgnoreCase
        );

    for (const QFileInfo &file : files) {
        QTreeWidgetItem *fileItem;

        if (parentItem) {
            fileItem = new QTreeWidgetItem(parentItem);
        } else {
            fileItem = new QTreeWidgetItem(ui->noteTree);
        }

        fileItem->setText(0, file.completeBaseName());
        fileItem->setData(0, Qt::UserRole, file.absoluteFilePath());
    }
}

void MainWindow::deleteNote(){

    QTreeWidgetItem* item = ui->noteTree->currentItem();

    if(!item){
        QMessageBox::information(this, "提示", "请先在左侧选择一篇笔记。");
        return;
    }

    QString filePath =item->data(0, Qt::UserRole).toString();

    // 文件夹没有存放笔记路径
    if (filePath.isEmpty()) {
        QMessageBox::information(this, "提示", "请选择笔记文件。");
        return;
    }

    const QString title = QFileInfo(filePath).completeBaseName();
    const auto answer = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除“%1”吗？此操作会删除磁盘上的文件。").arg(title),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (answer != QMessageBox::Yes) {
        return;
    }

    // 收到确认删除消息，开始删除文件
    QFile file(filePath);

    if (!file.remove()) {
        QMessageBox::warning(
            this,
            "删除失败",
            file.errorString()
            );
        return;
    }

    if (filePath == currentFilePath) {
        currentFilePath.clear();
        ui->editor->clear();
        ui->editor->document()->setModified(false);
        ui->preview->clear();
    }

    refreshVault();



}

void MainWindow::searchNote(){
    if (vaultPath.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择笔记库文件夹。");
        return;
    }

    bool ok = false;
    const QString keyword = QInputDialog::getText(
                                this,
                                "搜索笔记",
                                "搜索标题或正文：",
                                QLineEdit::Normal,
                                QString(),
                                &ok
                                ).trimmed();

    if (!ok || keyword.isEmpty()) {
        return;
    }

    if (!maybeSave()) {
        return;
    }

    ui->noteTree->clear();

    QDirIterator it(    // 边遍历边操作
        vaultPath,
        QStringList{"*.md"},
        QDir::Files | QDir::NoSymLinks,
        QDirIterator::Subdirectories
        );

    int matchCount = 0;
    while (it.hasNext()) {
        const QString filePath = it.next();
        const QFileInfo info(filePath);

        bool matches = info.completeBaseName().contains(
            keyword, Qt::CaseInsensitive
            );

        if(!matches){
            QFile file(filePath);

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qDebug() << "打开失败：" << file.errorString();
                return;
            }

            QByteArray data = file.readAll();
            QString text = QString::fromUtf8(data);

            matches = text.contains(
                keyword, Qt::CaseInsensitive
                );
        }

        if(matches){
            auto* item = new QTreeWidgetItem(ui->noteTree);
            item->setText(0, info.completeBaseName());
            item->setData(0, Qt::UserRole, filePath);

            item->setToolTip(
                0, QDir(vaultPath).relativeFilePath(filePath)
                );
            matchCount++;
        }
    }
    if (matchCount == 0) {
        QMessageBox::information(this, "搜索结果", "没有找到匹配的笔记。");
    }

}
