#include "prosetpage.h"
#include "ui_prosetpage.h"
#include <QLineEdit>
#include <QDir>
#include <QFileDialog>

/*
 * 项目设置向导页面的构造函数。
 * 它初始化界面，注册项目路径和项目名称字段到向导系统以便数据传递，
 * 并将文本框的编辑信号连接到完成状态检查。同时设置初始路径为当前工作目录，
 * 并启用文本框的清除按钮功能。
*/
ProSetPage::ProSetPage(QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::ProSetPage)
{
    ui->setupUi(this);
    // 注册字段到向导系统，便于页面间数据传递
    registerField("proPath", ui->lineEdit_2);
    registerField("proName*", ui->lineEdit);
    // 连接信号槽：当文本框内容编辑时触发完成状态检查
    connect(ui->lineEdit, &QLineEdit::textEdited, this, &ProSetPage::completeChanged);
    connect(ui->lineEdit_2, &QLineEdit::textEdited, this, &ProSetPage::completeChanged);
    // 设置初始路径为当前工作目录
    QString curPath = QDir::currentPath();
    ui->lineEdit_2->setText(curPath);
    // 将光标移动到文本末尾
    ui->lineEdit_2->setCursorPosition(ui->lineEdit_2->text().size());
    // 启用清除按钮（文本框右侧的X按钮）
    ui->lineEdit_2->setClearButtonEnabled(true);
    ui->lineEdit->setClearButtonEnabled(true);
}

ProSetPage::~ProSetPage()
{
    delete ui;
}

/*
 * 获取项目设置数据的函数。
 * 它从界面上的两个文本框中分别读取项目名称和项目路径，
 * 并通过引用参数返回这两个值。
 */
void ProSetPage::GetProSettings(QString &name, QString &path)
{
    name = ui->lineEdit->text();
    path = ui->lineEdit_2->text();
}

/*
 * 验证向导页面是否完成的函数。
 * 它检查项目名称和路径是否为空，验证指定路径是否存在，并检查该路径下是否已存在同名项目。
 * 如果任何一项检查失败，则在提示标签显示错误信息并返回false；
 * 所有验证通过后清空提示信息并返回完成状态。
 */
bool ProSetPage::isComplete() const
{
    // 检查项目名和路径是否为空
    if(ui->lineEdit->text() == "" || ui->lineEdit_2->text() == ""){
        return false;
    }

    // 检查路径是否存在
    QDir dir(ui->lineEdit_2->text());
    if(!dir.exists()){
        ui->tips->setText("project path is not exists");
        return false;
    }

    // 检查项目是否已存在（拼接路径+项目名）
    QString absFilePath= dir.absoluteFilePath(ui->lineEdit->text());
    QDir dist_dir(absFilePath);
    if(dist_dir.exists()){
        ui->tips->setText("project has exists, change path or name");
        return false;
    }

    // 所有验证通过，清空提示信息
    ui->tips->setText("");
    // 返回父类的完成状态（通常为true）
    return QWizardPage::isComplete();
}

/*
 * 处理按钮点击事件的槽函数。
 * 它打开一个文件对话框让用户选择目录，并将选中的路径显示在界面上的路径输入框中。
 */
void ProSetPage::on_pushButton_clicked()
{
    // 创建一个文件对话框对象
    QFileDialog file_dialog;
    // 设置选择模式为“目录” 而不是单个文件
    file_dialog.setFileMode(QFileDialog::Directory);
    // 设置对话框标题
    file_dialog.setWindowTitle("选择导入的文件夹");
    // 获取当前程序工作路径， 并设置为对话框的默认打开目录
    auto path = QDir::currentPath();
    file_dialog.setDirectory(path);
    // 设置对话框的显示模式为详细信息模式
    file_dialog.setViewMode(QFileDialog::Detail);

    // 存放用户选择的路径
    QStringList fileNames;
    // 打开对话框（模态执行），用户点击确定返回true
    if(file_dialog.exec()){
        // 获取用户选中的路径
        fileNames = file_dialog.selectedFiles();
    }

    // 若用户没有选择任何路径，直接返回
    if(fileNames.length() <= 0){
        return;
    }
    // 取出第一个选中的目录路径
    QString import_path = fileNames.at(0);
    // 把选中的目录路径显示到界面上的输入框中
    ui->lineEdit_2->setText(import_path);

}

