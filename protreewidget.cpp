#include "protreewidget.h"
#include <QDir>
#include "protreeitem.h"
#include "const.h"
#include <QGuiApplication>
#include <QMenu>
#include <QFileDialog>
#include "removeprodialog.h"

ProTreeWidget::ProTreeWidget(QWidget *parent):QTreeWidget(parent),
    _right_btn_item(nullptr), _active_item(nullptr), _dialog_progress(nullptr),_selected_item(nullptr),
    _thread_create_pro(nullptr), _thread_open_pro(nullptr),_open_progressdlg(nullptr)

{
    // 隐藏树控件的表头（不显示列标题），更像一个文件浏览树
    this->setHeaderHidden(true);

    // 连接信号槽：当用户点击树节点时，触发 SlotItemPressed 函数
    connect(this, &ProTreeWidget::itemPressed, this, &ProTreeWidget::SlotItemPressed);

    // 创建右键菜单的动作（Action）
    _action_import = new QAction(QIcon("/icon/import.png"), tr("导入文件"), this);
    // 图标：import.png，显示文本：导入文件
    _action_setstart = new QAction(QIcon(":/icon/core.png"), tr("设置活动项目"), this);
    // 图标：core.png，显示文本：设置活动项目
    _action_closepro = new QAction(QIcon(":/icon/close.png"), tr("关闭项目"), this);
    // 图标：close.png，显示文本：关闭项目
    _action_slideshow = new QAction(QIcon(":/icon/slideshow.png"), tr("轮播图播放"), this);
    // 图标：slideshow.png，显示文本：轮播图播放

    // 连接动作触发信号与槽函数
    // 当用户点击“导入文件”菜单项时，触发 SlotImport() 槽函数
    connect(_action_import, &QAction::triggered, this, &ProTreeWidget::SlotImport);

    connect(_action_setstart, &QAction::triggered, this, &ProTreeWidget::SlotSetActive);

    connect(_action_closepro, &QAction::triggered, this, &ProTreeWidget::SlotClosePro);

}

void ProTreeWidget::AddProTree(const QString &name, const QString &path)
{
    // 生成项目的完整路径 = path + / + name
    QDir dir(path);
    QString file_path = dir.absoluteFilePath(name);
    // 如果这个路径已经存在于集合中， 直接返回（防止重复添加）
    if(_set_path.find(file_path) != _set_path.end()){
        return;
    }
    // 检查项目路径是否存在，不存在就创建
    QDir pro_dir(file_path);
    if(!pro_dir.exists()){
        // 递归创建目录
        bool enable = pro_dir.mkpath(file_path);
        if(!enable){
            return;
        }
    }

    // 将路径加入集合， 避免后续重复添加
    _set_path.insert(file_path);
    // 创建一个自定义的树节点
    auto * item = new ProTreeItem(this, name, file_path, TreeItemPro);
    // 设置节点显示的文字
    item->setData(0, Qt::DisplayRole, name);
    // 设置节点显示的图标
    item->setData(0, Qt::DecorationRole, QIcon(":/icon/dir.png"));
    // 设置鼠标悬停时的提示信息
    item->setData(0, Qt::ToolTipRole, file_path);
    // 将新节点添加为顶层节点
    this->addTopLevelItem(item);
}

// 当用户点击树节点时触发
void ProTreeWidget::SlotItemPressed(QTreeWidgetItem *pressedItem, int column)
{
    // 判断是否为鼠标右键点击
    if(QGuiApplication::mouseButtons() == Qt::RightButton){
        QMenu menu(this);  // 创建右键菜单
        int itemtype = pressedItem->type();  // 获取节点类型
        if(itemtype == TreeItemPro){  // 如果是项目类型节点
            _right_btn_item = pressedItem; // 记录当前右键点击的节点，用于后续操作
            // 添加菜单操作项
            menu.addAction(_action_import);     // 导入文件夹
            menu.addAction(_action_setstart);   // 设置起始项
            menu.addAction(_action_closepro);   // 关闭项目
            menu.addAction(_action_slideshow);  // 幻灯片浏览
            menu.exec(QCursor::pos());          // 在鼠标当前位置显示菜单
        }
    }
}

// 导入文件夹操作的槽函数
void ProTreeWidget::SlotImport()
{
    QFileDialog file_dialog;                      // 文件夹选择对话框
    file_dialog.setFileMode(QFileDialog::Directory); // 只能选择文件夹
    file_dialog.setWindowTitle(tr("选择导入的文件夹"));

    QString path = "";
    if(!_right_btn_item){                         // 如果没有记录右键节点
        qDebug() << "_right_btn_item is empty" << Qt::endl;
        return;
    }

    // 获取右键节点的路径，设置为对话框初始路径
    path = dynamic_cast<ProTreeItem*>(_right_btn_item)->GetPath();
    file_dialog.setDirectory(path);
    file_dialog.setViewMode(QFileDialog::Detail); // 设置详细列表模式

    QStringList fileNames;
    if(file_dialog.exec()){                       // 打开对话框并执行
        fileNames = file_dialog.selectedFiles();  // 获取用户选择的文件夹
    }

    if(fileNames.length() <= 0){                 // 如果没有选择，直接返回
        return;
    }

    QString import_path = fileNames.at(0);       // 用户选择的导入路径
    int file_count = 0;                          // 初始化文件计数器
    _dialog_progress = new QProgressDialog(this); // 创建进度条对话框

    // 创建线程对象，负责扫描和复制文件
    _thread_create_pro = std::make_shared<ProTreeThread>(
        std::ref(import_path),  // 源路径
        std::ref(path),         // 目标路径（当前项目节点路径）
        _right_btn_item,        // 父节点
        file_count,             // 文件计数
        this,                   // QTreeWidget 指针
        _right_btn_item,        // 根节点
        nullptr                 // QThread 父对象
        );

    // 连接线程信号与槽函数，用于更新 UI
    connect(_thread_create_pro.get(), &ProTreeThread::SigUpdateProgress,
            this, &ProTreeWidget::SlotUpdateProgress);
    connect(_thread_create_pro.get(), &ProTreeThread::SigFinishProgress,
            this, &ProTreeWidget::SlotFinishProgress);
    connect(_dialog_progress, &QProgressDialog::canceled,
            this, &ProTreeWidget::SlotCancelProgress);
    connect(this, &ProTreeWidget::SigCancelProgress,
            _thread_create_pro.get(), &ProTreeThread::SlotCancelProgress);

    // 启动线程（开始扫描和复制文件）
    _thread_create_pro->start();

    // 配置进度条对话框
    _dialog_progress->setWindowTitle("Please wait...");
    _dialog_progress->setFixedWidth(PROGRESS_WIDTH); // 固定宽度
    _dialog_progress->setRange(0, PROGRESS_WIDTH);   // 设置进度范围
    _dialog_progress->exec();                         // 显示对话框并阻塞等待线程完成
}

void ProTreeWidget::SlotSetActive()
{
    if(!_right_btn_item){
        return;
    }

    QFont nullFont;
    nullFont.setBold(false);
    if(_active_item){
        _active_item->setFont(0, nullFont);
    }

    _active_item = _right_btn_item;
    nullFont.setBold(true);
    _active_item->setFont(0, nullFont);
}

void ProTreeWidget::SlotClosePro()
{
    RemoveProDialog remove_pro_dialog;
    auto res = remove_pro_dialog.exec();
    if(res != QDialog::Accepted){
        return;
    }
    bool b_remove = remove_pro_dialog.IsRemoved();
    auto index_right_btn = this->indexOfTopLevelItem(_right_btn_item);
    auto * protreeitem = dynamic_cast<ProTreeItem*>(_right_btn_item);
    // auto * selecteditem = dynamic_cast<ProTreeItem>(_selected_item);
    auto delete_path = protreeitem->GetPath();
    _set_path.remove(delete_path);
    if(b_remove){
        QDir delete_dir(delete_path);
        delete_dir.removeRecursively();
    }

    if(protreeitem == _active_item){
        _active_item = nullptr;
    }

    delete this->takeTopLevelItem(index_right_btn);
    _right_btn_item = nullptr;
}

// 线程每处理一个文件/文件夹，会发出的进度更新信号
void ProTreeWidget::SlotUpdateProgress(int count)
{
    if(!_dialog_progress){  // 如果进度条不存在，直接返回
        return;
    }

    // 防止超过最大值，使用取模处理
    if(count >= PROGRESS_MAX){
        _dialog_progress->setValue(count % PROGRESS_MAX);
    } else {
        _dialog_progress->setValue(count);
    }
}

// 线程完成任务时，更新进度条并删除对话框
void ProTreeWidget::SlotFinishProgress()
{
    _dialog_progress->setValue(PROGRESS_MAX);  // 设置为最大值
    _dialog_progress->deleteLater();           // 延迟删除对话框（安全删除）
}

// 用户点击进度条取消按钮时调用
void ProTreeWidget::SlotCancelProgress()
{
    emit SigCancelProgress();                  // 发信号通知线程停止
    delete _dialog_progress;                   // 删除对话框
    _dialog_progress = nullptr;                // 防止悬空指针
}

void ProTreeWidget::SlotUpOpenProgress(int count)
{
    if(!_open_progressdlg){  // 如果进度条不存在，直接返回
        return;
    }

    // 防止超过最大值，使用取模处理
    if(count >= PROGRESS_MAX){
        _open_progressdlg->setValue(count % PROGRESS_MAX);
    } else {
        _open_progressdlg->setValue(count);
    }
}

void ProTreeWidget::SlotFinishOpenProgress()
{
    if(!_open_progressdlg){  // 如果进度条不存在，直接返回
        return;
    }
    _open_progressdlg->setValue(PROGRESS_MAX);
    delete _open_progressdlg;
    _open_progressdlg = nullptr;
}

void ProTreeWidget::SlotCancelOpenProgress()
{
    emit SigCancelOpenProgress();                  // 发信号通知线程停止
    delete _open_progressdlg;                   // 删除对话框
    _open_progressdlg = nullptr;                // 防止悬空指针
}

// 打开项目
void ProTreeWidget::SlotOpenPro(const QString &path)
{
    // 如果路径已经在集合中，说明项目已经打开过，直接返回，不再重复打开
    if(_set_path.find(path) != _set_path.end()){
        return;
    }

    // 将路径插入集合，防止重复打开
    _set_path.insert(path);

    int file_count = 0;               // 用于统计打开项目中的文件数量
    QDir pro_dir(path);               // 创建一个 QDir 对象，用于操作目录
    QString proname = pro_dir.dirName(); // 获取目录名称，即项目名称

    // 创建一个线程对象，用于递归遍历目录，加载项目树
    // std::make_shared 创建一个 shared_ptr，确保线程对象在使用过程中不会被释放
    _thread_open_pro = std::make_shared<OpenTreeThread>(path, file_count, this, nullptr);

    // 创建一个进度对话框，用于显示打开项目的进度
    _open_progressdlg = new QProgressDialog(this);

    // 连接线程的信号与槽函数
    // 当线程更新进度时，调用 SlotUpOpenProgress 更新 UI
    connect(_thread_open_pro.get(), &OpenTreeThread::SigUpdateProgress,
            this, &ProTreeWidget::SlotUpOpenProgress);
    // 当线程完成操作时，调用 SlotFinishOpenProgress
    connect(_thread_open_pro.get(), &OpenTreeThread::SigFinishProgress,
            this, &ProTreeWidget::SlotFinishOpenProgress);

    // 当用户点击进度对话框的取消按钮时，触发槽函数取消线程操作
    connect(_open_progressdlg, &QProgressDialog::canceled,
            this, &ProTreeWidget::SlotCancelOpenProgress);
    // 自定义信号，发射后通知线程停止处理
    connect(this, &ProTreeWidget::SigCancelOpenProgress,
            _thread_open_pro.get(), &OpenTreeThread::SlotCancelProgress);

    // 启动线程，开始处理目录遍历
    _thread_open_pro->start();

    // 配置进度对话框显示属性
    _open_progressdlg->setWindowTitle("Please wait..."); // 设置对话框标题
    _open_progressdlg->setFixedWidth(PROGRESS_WIDTH);    // 固定宽度，防止被拉伸
    _open_progressdlg->setRange(0, PROGRESS_WIDTH);      // 设置进度条范围
    _open_progressdlg->exec();                           // 显示对话框并阻塞当前线程，直到对话框关闭
}


