#include "registerdialog.h"
#include <QLabel>
#include <QMessageBox>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Регистрация нового оператора");
    setMinimumWidth(400);
    setModal(true); // Модальное окно — блокирует родительское

    // === Поля формы ===
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Например: Иванов А.С.");
    m_nameEdit->setStyleSheet("padding: 5px;");

    m_roleCombo = new QComboBox(this);
    m_roleCombo->addItems({"Оператор", "Старший оператор", "Администратор"});
    m_roleCombo->setStyleSheet("padding: 5px;");

    // === Кнопки ===
    m_okBtn = new QPushButton("Зарегистрировать", this);
    m_okBtn->setStyleSheet(
        "QPushButton { background-color: #00AA00; color: white; padding: 8px; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #009900; }");

    m_cancelBtn = new QPushButton("Отмена", this);
    m_cancelBtn->setStyleSheet(
        "QPushButton { background-color: #666666; color: white; padding: 8px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #555555; }");

    // === Компоновка ===
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("ФИО оператора:", m_nameEdit);
    formLayout->addRow("Роль:", m_roleCombo);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    // === Сигналы и слоты ===
    connect(m_okBtn, &QPushButton::clicked, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Пожалуйста, введите ФИО оператора");
            return;
        }
        accept(); // Закрываем диалог с кодом QDialog::Accepted
    });

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

QString RegisterDialog::operatorName() const {
    return m_nameEdit->text().trimmed();
}

QString RegisterDialog::operatorRole() const {
    return m_roleCombo->currentText();
}
