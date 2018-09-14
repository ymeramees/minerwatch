#include "kaevuririda.h"

KaevuriRida::KaevuriRida(QWidget *parent) : QHBoxLayout(parent)
{
    restartNo = 0;
    nameLabel = new QLabel();
    portLabel = new QLabel();
    timeLabel = new QLabel(QTime::currentTime().toString("hh:mm:ss"));
    statusLabel = new QLabel("teadmata");
    restartButton = new QPushButton("Restart");
    connect(restartButton, SIGNAL(pressed()), this, SLOT(orderRestart()));

    this->addWidget(nameLabel);
    this->addWidget(portLabel);
    this->addWidget(timeLabel);
    this->addWidget(statusLabel);
    this->addWidget(restartButton);
}

void KaevuriRida::alive()
{
    timeLabel->setText(QTime::currentTime().toString("hh:mm:ss"));
    statusLabel->setText("alive");
    restartNo = 0;
}

void KaevuriRida::orderRestart()
{
    emit orderRestart(portLabel->text());    //Saadab pesa nr'i, millisele restart teha
}
