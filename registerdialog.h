#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr, bool *is_first = nullptr);

    // Геттеры для получения данных после закрытия диалога
    QString operatorName() const;
    QString operatorRole() const;

private:
    QLineEdit *m_nameEdit;
    QComboBox *m_roleCombo;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
};

#endif // REGISTERDIALOG_H
