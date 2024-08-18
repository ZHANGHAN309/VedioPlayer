#include "vedioplayer.h"
#include "ui_vedioplayer.h"

VedioPlayer::VedioPlayer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VedioPlayer)
{
    ui->setupUi(this);
}

VedioPlayer::~VedioPlayer()
{
    delete ui;
}

