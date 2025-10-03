#include "protree.h"
#include "ui_protree.h"

/*
 * 项目树控件的构造函数。
 * 它初始化界面并设置对话框的固定宽度为378像素
 */
ProTree::ProTree(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProTree)
{
    ui->setupUi(this);
    this->setMinimumWidth(378);
    this->setMaximumWidth(378);
}

ProTree::~ProTree()
{
    delete ui;
}

/*
 * 向项目树中添加新项目的函数。
 * 它接收项目名称和路径作为参数，并调用自定义树控件的方法将项目添加到树形结构中。
 */
void ProTree::AddProToTree(const QString name, const QString path)
{
    // 将新项目添加到 treeWidget 中
    // 注意：这里调用的是 ui->treeWidget 的自定义函数 AddProTree
    //       说明 treeWidget 不是标准的 QTreeWidget，
    //       而是你自己继承 QTreeWidget 并扩展了 AddProTree 方法
    ui->treeWidget->AddProTree(name, path);
}
