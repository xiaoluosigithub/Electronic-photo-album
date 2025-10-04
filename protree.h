#ifndef PROTREE_H
#define PROTREE_H

#include <QDialog>
#include <QTreeWidget>

namespace Ui {
class ProTree;
}

class ProTree : public QDialog
{
    Q_OBJECT

public:
    // 构造 ProTree 的时候，可以选择把另一个 QWidget 作为它的父控件。
    explicit ProTree(QWidget *parent = nullptr);
    ~ProTree();
    QTreeWidget* GetTreeWidget();
private:
    Ui::ProTree *ui;
public slots:
    void AddProToTree(const QString name, const QString path);


};

#endif // PROTREE_H
