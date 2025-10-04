#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenu>
#include <QAction>
#include <QDebug>
#include "wizard.h"
#include "protree.h"
#include <QFileDialog>
#include "protreewidget.h"

/*
 * 这是主窗口的构造函数，负责初始化用户界面。它创建了文件菜单和设置菜单，
 * 并添加了创建项目、打开项目和背景音乐等菜单项及其快捷键。
 * 同时将创建项目的动作信号连接到对应的槽函数，并初始化了项目树控件，将其添加到主界面的布局中。
*/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 创建菜单
    QMenu * menu_file = menuBar()->addMenu(tr("文件(&F)"));

    // 创建项目动作
    QAction * act_create_pro = new QAction(QIcon(":/icon/createpro.png"), tr("创建项目"), this);
    act_create_pro->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
    menu_file->addAction(act_create_pro);

    // 打开项目动作
    QAction * act_open_pro = new QAction(QIcon(":/icon/openpro.png"), tr("打开项目"), this);
    act_open_pro->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
    menu_file->addAction(act_open_pro);

    // 创建设置菜单
    QMenu * menu_set = menuBar()->addMenu(tr("设置(&S)"));
    // 设置背景音乐
    QAction * act_music = new QAction(QIcon(":/icon/music.png"), tr("背景音乐"), this);
    act_music->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    menu_set->addAction(act_music);

    // 连接信号和槽

    // 创建项目
    connect(act_create_pro, &QAction::triggered, this, &MainWindow::SlotCreatePro);
    // 打开项目
    connect(act_open_pro, &QAction::triggered, this, &MainWindow::SlotOpenPro);

    // 创建项目树
    _protree = new ProTree();
    // 将项目树加载到页面上
    ui->proLayout->addWidget(_protree);

    // 拿到项目树页面中的项目树窗口
    QTreeWidget* tree_widget = dynamic_cast<ProTree*>(_protree)->GetTreeWidget();
    auto * pro_tree_widget = dynamic_cast<ProTreeWidget*>(tree_widget);

    connect(this, &MainWindow::SigOpenPro, pro_tree_widget, &ProTreeWidget::SlotOpenPro);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*
 * 用于创建项目的Qt槽函数。它弹出一个向导对话框让用户配置新项目，
 * 并将配置完成后的结果信号连接到项目树，以在界面中添加新的项目节点。
*/
void MainWindow::SlotCreatePro(bool)
{
    qDebug() << "slot create pro triggered" << Qt::endl;
    // 创建一个向导对话框， 设置父窗口为MainWindow
    Wizard wizard(this);
    // 设置向导窗口标题
    wizard.setWindowTitle(tr("创建项目"));
    // 获取向导窗口第0页
    auto * page = wizard.page(0);
    // 设置第0页的标题
    page->setTitle(tr("设置项目配置"));

    // 将向导中的SigProSetting 信号连接到ProTree的AddProToTree槽
    // 作用：当用户在向导中完成配置时，把结果传回ProTree，在树结构中添加项目节点
    connect(&wizard, &Wizard::SigProSettings, dynamic_cast<ProTree*>(_protree), &ProTree::AddProToTree);
    wizard.show();
    wizard.exec();
    // 断开所有信号 todo...
}

// MainWindow 类的槽函数，用于打开一个项目目录
void MainWindow::SlotOpenPro(bool)
{
    // 创建一个文件对话框对象
    QFileDialog file_dialog;
    // 设置文件模式为选择目录（而不是单个文件）
    file_dialog.setFileMode(QFileDialog::Directory);
    // 设置对话框标题
    file_dialog.setWindowTitle(tr("选择导入的文件夹"));
    // 设置对话框默认打开的路径为当前程序的运行目录
    file_dialog.setDirectory(QDir::currentPath());
    // 设置文件视图模式为详细信息（列表+属性）
    file_dialog.setViewMode(QFileDialog::Detail);

    QStringList fileNames;
    // 打开文件对话框，如果用户点击了“确定”按钮（exec()返回非0值）
    if(file_dialog.exec()){
        // 获取用户选择的目录（可能是多个，但这里只取第一个）
        fileNames = file_dialog.selectedFiles();
    }

    // 如果用户没有选择任何目录，则直接返回
    if(fileNames.length() <= 0){
        return;
    }

    // 获取第一个用户选择的路径
    QString import_path = fileNames.at(0);
    // 发出信号，将选择的目录路径传递出去
    emit SigOpenPro(import_path);
}




