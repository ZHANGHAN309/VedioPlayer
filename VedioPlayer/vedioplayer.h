#ifndef VEDIOPLAYER_H
#define VEDIOPLAYER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class VedioPlayer; }
QT_END_NAMESPACE

class VedioPlayer : public QWidget
{
    Q_OBJECT

public:
    VedioPlayer(QWidget *parent = nullptr);
    ~VedioPlayer();

private:
    Ui::VedioPlayer *ui;
};
#endif // VEDIOPLAYER_H
