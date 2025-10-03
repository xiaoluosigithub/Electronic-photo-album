#include "wizard.h"
#include "ui_wizard.h"

Wizard::Wizard(QWidget *parent)
    : QWizard(parent)
    , ui(new Ui::Wizard)
{
    ui->setupUi(this);
}

Wizard::~Wizard()
{
    delete ui;
}

void Wizard::done(int result)
{
    // 如果用户点击了“取消”或关闭对话框
    if(result == QDialog::Rejected){
        // 不做额外处理，直接调用父类的 done()，关闭向导
        return QWizard::done(result); // 子类只是拦截了特定情况，其余交给父类。
    }

    // 如果用户完成了向导
    QString name, path;
    // 从第一个向导页面获取项目配置（项目名和路径）
    ui->wizardPage1->GetProSettings(name, path);
    // 发出自定义信号，把配置传递给外部
    emit SigProSettings(name, path);
    // 调用父类的done 结束对话框
    QWizard::done(result);
}
