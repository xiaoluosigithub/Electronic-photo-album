#include "opentreethread.h"
#include <QDir>
#include "protreeitem.h"
#include "const.h"

OpenTreeThread::OpenTreeThread(const QString &src_path, int file_count,
                               QTreeWidget *self, QObject *parent)
    :QThread(parent), _bstop(false), _src_path(src_path), _file_count(file_count),
    _self(self), _root(nullptr)
{

}

void OpenTreeThread::OpenProTree(
    const QString &src_path,   // 项目根目录路径
    int &file_count,           // 文件计数引用，用于统计文件和目录数量
    QTreeWidget *self          // 树控件指针，用于添加节点
    )
{
    QDir src_dir(src_path);               // 创建 QDir 对象，用于操作目录
    auto name = src_dir.dirName();        // 获取目录名称，作为项目名

    // 创建一个表示项目根节点的 ProTreeItem
    auto * item = new ProTreeItem(self, name, src_path, TreeItemPro);
    item->setData(0, Qt::DisplayRole, name);              // 设置显示名称
    item->setData(0, Qt::DecorationRole, QIcon(":/icon/dir/png")); // 设置图标为文件夹图标
    item->setData(0, Qt::ToolTipRole, src_path);         // 设置鼠标悬停提示为完整路径

    _root = item;  // 保存根节点指针，供递归使用

    // 调用递归函数，遍历项目目录，构建树结构
    // 参数说明：
    // src_path  : 当前目录路径
    // file_count: 文件计数
    // self      : 树控件
    // _root     : 项目根节点
    // item      : 当前父节点（这里是根节点）
    // nullptr   : preitem（前一个节点指针，根节点没有前驱）
    RecursiveProTree(src_path, file_count, self, _root, item, nullptr);
}


void OpenTreeThread::run()
{
    OpenProTree(_src_path, _file_count, _self);
    // 如果线程在中途被取消
    if(_bstop){
        // 获取根节点的路径
        auto path = dynamic_cast<ProTreeItem*>(_root)->GetPath();
        // 从树控件中删除对应的顶层节点
        auto index = _self->indexOfTopLevelItem(_root);
        delete _self->takeTopLevelItem(index);
        // 删除目标目录（递归删除所有子文件/文件夹）
        QDir dir(path);
        dir.removeRecursively();
        return;
    }

    // 如果成功完成，发送完成信号
    emit SigFinishProgress(_file_count);
}


void OpenTreeThread::RecursiveProTree(
    const QString &src_path,       // 当前要遍历的路径
    int &file_count,               // 文件计数引用，用于统计文件和目录数量
    QTreeWidget *self,             // 树控件指针（暂未在此函数中直接使用）
    QTreeWidgetItem *root,         // 树的根节点
    QTreeWidgetItem *parent,       // 当前递归层的父节点
    QTreeWidgetItem *preitem       // 前一个节点指针，用于链表式管理节点关系
    )
{
    QDir src_dir(src_path);                         // 创建目录对象
    src_dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot); // 过滤目录和文件，但排除 "." 和 ".."
    src_dir.setSorting(QDir::Name);                // 按名称排序
    QFileInfoList list = src_dir.entryInfoList();  // 获取当前目录下的文件和子目录信息列表

    // 遍历目录下所有条目
    for(int i = 0; i < list.size(); ++i){
        if(_bstop){    // 检查线程是否被取消
            return;
        }

        QFileInfo fileInfo = list.at(i);          // 当前文件或目录信息
        bool bIsDir = fileInfo.isDir();           // 判断是否是目录

        if(bIsDir){  // 如果是目录
            if(_bstop){
                return;
            }
            file_count ++;                        // 文件计数加 1
            emit SigUpdateProgress(file_count);   // 发射信号更新进度条

            // 创建一个树节点表示目录
            auto * item = new ProTreeItem(
                _root,
                fileInfo.fileName(),
                fileInfo.absoluteFilePath(),
                _root,
                TreeItemDir
                );
            item->setData(0, Qt::DisplayRole, fileInfo.fileName());        // 显示名称
            item->setData(0, Qt::DecorationRole, QIcon(":/icon/dir/png")); // 设置目录图标
            item->setData(0, Qt::ToolTipRole, fileInfo.absoluteFilePath());// 提示显示完整路径

            // 递归遍历子目录
            RecursiveProTree(
                fileInfo.absoluteFilePath(),
                file_count,
                self,
                _root,
                item,     // 当前目录作为父节点
                preitem   // 前一个节点指针传入
                );
        } else {    // 如果是文件
            if(_bstop){
                return;
            }

            const QString& suffix = fileInfo.completeSuffix(); // 获取文件后缀
            if(suffix != "png" && suffix != "jpeg" && suffix != "jpg"){
                continue;   // 只处理图片文件，其他文件忽略
            }

            file_count ++;                        // 文件计数加 1
            emit SigUpdateProgress(file_count);   // 发射信号更新进度条

            // 创建树节点表示图片文件
            auto * item = new ProTreeItem(
                _root,
                fileInfo.fileName(),
                fileInfo.absoluteFilePath(),
                _root,
                TreeItemPic
                );
            item->setData(0, Qt::DisplayRole, fileInfo.fileName());        // 显示名称
            item->setData(0, Qt::DecorationRole, QIcon(":/icon/pic/png")); // 设置图片图标
            item->setData(0, Qt::ToolTipRole, fileInfo.absoluteFilePath());// 提示显示完整路径

            // 将节点与前一个节点连接，形成链表关系
            if(preitem){
                auto * pre_proitem = dynamic_cast<ProTreeItem*>(preitem);
                pre_proitem->SetNextItem(item); // 设置前一个节点的下一个节点指针
            }
            item->SetPreItem(preitem);          // 设置当前节点的前驱节点
            preitem = item;                     // 更新前一个节点指针
        }
    }

    // 当前目录遍历完成后，发射完成信号
    emit SigFinishProgress(file_count);
}


void OpenTreeThread::SlotCancelProgress()
{
    this->_bstop = true;
}
