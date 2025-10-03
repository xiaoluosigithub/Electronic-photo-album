#include "protreethread.h"
#include <QDir>
#include "protreeitem.h"
#include "const.h"

// 构造函数：初始化线程任务参数
ProTreeThread::ProTreeThread(const QString &src_path,
                             const QString &dist_path,
                             QTreeWidgetItem *parentItem,
                             int file_count, QTreeWidget *self,
                             QTreeWidgetItem *root, QObject *parent)
    :QThread(parent),             // 继承 QThread，启用线程机制
    _src_path(src_path),         // 源目录路径（原始项目路径）
    _dist_path(dist_path),       // 目标目录路径（拷贝后保存的路径）
    _file_count(file_count),     // 文件计数器（用于进度统计）
    _parent_item(parentItem),    // 父节点（ProTreeItem）
    _self(self),                 // 树控件 QTreeWidget
    _root(root),                 // 根节点（顶层 ProTreeItem）
    _bstop(false)                // 停止标记，默认为 false
{

}

ProTreeThread::~ProTreeThread()
{

}

// 线程执行函数
void ProTreeThread::run()
{
    // 创建项目树（递归扫描目录并填充节点）
    CreateProTree(_src_path, _dist_path, _parent_item, _file_count, _self, _root);

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

// 核心递归函数：遍历目录并构建项目树
void ProTreeThread::CreateProTree(const QString &src_path,
                                  const QString &dist_path,
                                  QTreeWidgetItem *parent_item,
                                  int &file_count,
                                  QTreeWidget *self,
                                  QTreeWidgetItem *root,
                                  QTreeWidgetItem *preItem)
{
    // 如果被取消，直接退出
    if(_bstop){
        return;
    }

    // needcopy 标志：当源目录和目标目录相同时，就不需要复制
    bool needcopy = true;
    if(src_path == dist_path){
        needcopy = false;
    }

    // 打开源目录，准备遍历
    QDir import_dir(src_path);
    QStringList nameFilters;
    import_dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot); // 过滤掉 "." 和 ".."
    import_dir.setSorting(QDir::Name);  // 按名称排序
    QFileInfoList list = import_dir.entryInfoList();

    // 遍历目录内容
    for(int i = 0; i < list.size(); ++i){
        if(_bstop){
            return;
        }

        QFileInfo fileInfo = list.at(i);
        bool bIsDir = fileInfo.isDir();

        if(bIsDir){ // 如果是目录
            if(_bstop){
                return;
            }

            // 更新进度
            file_count ++;
            emit SigUpdateProgress(file_count);

            // 构建目标目录路径
            QDir dist_dir(_dist_path);
            QString sub_dist_path = dist_dir.absoluteFilePath(fileInfo.fileName());
            QDir sub_dist_dir(sub_dist_path);
            if(!sub_dist_dir.exists()){
                bool ok = sub_dist_dir.mkpath(sub_dist_path); // 创建目录
                if(!ok){
                    continue; // 如果失败则跳过
                }
            }

            // 创建一个 ProTreeItem 节点（目录类型）
            auto *item = new ProTreeItem(parent_item, fileInfo.fileName(), sub_dist_path,
                                         root, TreeItemDir);
            item->setData(0, Qt::DisplayRole, fileInfo.fileName());  // 显示名称
            item->setData(0, Qt::DecorationRole, QIcon(":/icon/dir.png")); // 设置图标
            item->setData(0, Qt::ToolTipRole, sub_dist_path); // 提示信息

            // 递归处理子目录
            CreateProTree(fileInfo.absoluteFilePath(), sub_dist_path,
                          item, file_count, self, root, preItem);

        } else { // 如果是文件
            if(_bstop){
                return;
            }

            // 只处理图片文件
            const QString& suffix = fileInfo.completeSuffix();
            if(suffix != "png" && suffix != "jpeg" && suffix != "jpg"){
                continue;
            }

            file_count ++;
            emit SigUpdateProgress(file_count);

            // 如果不需要复制（源目录=目标目录），就直接跳过复制操作
            if(!needcopy){
                continue;
            }

            // 构造目标文件路径，并复制文件
            QDir dist_dir(dist_path);
            QString dist_file_path = dist_dir.absoluteFilePath(fileInfo.fileName());
            if(!QFile::copy(fileInfo.absoluteFilePath(), dist_file_path)){
                continue; // 如果复制失败则跳过
            }

            // 创建一个 ProTreeItem 节点（图片类型）
            auto * item = new ProTreeItem(parent_item, fileInfo.fileName(),
                                         dist_file_path, root, TreeItemPic);
            item->setData(0, Qt::DisplayRole, fileInfo.fileName());
            item->setData(0, Qt::DecorationRole, QIcon(":/icon/pic.png"));
            item->setData(0, Qt::ToolTipRole, dist_file_path);

            // 链接前后兄弟节点（实现类似双向链表）
            if(preItem){
                auto * pre_proitem = dynamic_cast<ProTreeItem*>(preItem);
                pre_proitem->SetNextItem(item);
            }
            item->SetPreItem(preItem);
            preItem = item;
        }
    }
}

// 槽函数：外部调用时设置停止标记
void ProTreeThread::SlotCancelProgress()
{
    this->_bstop = true;
}
