#ifndef CONTACTDETAILS_H
#define CONTACTDETAILS_H

#include "ui_contactdetails.h"

#include <akonadi/item.h>

#include <QtGui/QWidget>
#include <QToolButton>
#include <QCalendarWidget>


class EditCalendarButton : public QToolButton
{
    Q_OBJECT
public:
    explicit EditCalendarButton( QWidget *parent = 0 );

    ~EditCalendarButton();

protected:
    virtual void mousePressEvent( QMouseEvent* e );

    friend class ContactDetails;
private:
    inline QCalendarWidget* calendarWidget() { return mCalendar; }

    QCalendarWidget *mCalendar;
};


class ContactDetails : public QWidget
{
    Q_OBJECT
public:
    explicit ContactDetails( QWidget *parent = 0 );

    ~ContactDetails();

    void setItem( const Akonadi::Item &item );
    void clearFields();
    inline QMap<QString, QString> contactData() {return mContactData;}

Q_SIGNALS:
    void saveContact();
    void modifyContact();

private:
    void initialize();
    EditCalendarButton *mCalendarButton;
    QMap<QString, QString> mContactData;
    bool mModifyFlag;
    Ui_ContactDetails mUi;

private Q_SLOTS:
    void slotEnableSaving();
    void slotSaveContact();
    void slotSetBirthday();
    void slotSetModifyFlag( bool );
};

#endif /* CONTACTDETAILS_H */

