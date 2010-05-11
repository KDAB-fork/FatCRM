#include "contactdetails.h"

#include <akonadi/item.h>

#include <kabc/addressee.h>
#include <kabc/address.h>

#include <QMouseEvent>

using namespace Akonadi;


EditCalendarButton::EditCalendarButton( QWidget *parent )
    : QToolButton( parent ), mCalendar(new QCalendarWidget())
{
    setText( tr( "&Edit" ) );
    setEnabled( false );
}

EditCalendarButton::~EditCalendarButton()
{
    delete mCalendar;
}


void EditCalendarButton::mousePressEvent( QMouseEvent* e )
{
    if ( mCalendar->isVisible() )
        mCalendar->hide();
    else {
        mCalendar->move( e->globalPos() );
        mCalendar->show();
        mCalendar->raise();
    }
}

ContactDetails::ContactDetails( QWidget *parent )
    : QWidget( parent )

{
    mUi.setupUi( this );
    initialize();
}

ContactDetails::~ContactDetails()
{
    delete mCalendarButton;
}

void ContactDetails::initialize()
{
    QList<QLineEdit*> lineEdits =
        mUi.contactInformationGB->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits )
        le->setReadOnly( true );
    mUi.description->setReadOnly( true );

    mCalendarButton = new EditCalendarButton(this);
    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget( mCalendarButton );
    mUi.calendarWidget->setLayout( buttonLayout );
    connect( mCalendarButton->calendarWidget(), SIGNAL( selectionChanged() ),
             this, SLOT( slotSetBirthday() ) );

    mModifyFlag = false;
    connect( mUi.saveButton, SIGNAL( clicked() ),
             this, SLOT( slotSaveContact() ) );
}

void ContactDetails::setItem (const Item &item )
{
    // contact info
    const KABC::Addressee addressee = item.payload<KABC::Addressee>();
    mUi.firstName->setText( addressee.givenName() );
    mUi.lastName->setText( addressee.familyName() );
    mUi.title->setText( addressee.title() );
    mUi.department->setText( addressee.department() );
    mUi.accountName->setText( addressee.organization() );
    mUi.primaryEmail->setText( addressee.preferredEmail() );
    mUi.homePhone->setText(addressee.phoneNumber( KABC::PhoneNumber::Home ).number() );
    mUi.mobilePhone->setText( addressee.phoneNumber( KABC::PhoneNumber::Cell ).number() );
    mUi.officePhone->setText( addressee.phoneNumber( KABC::PhoneNumber::Work ).number() );
    mUi.otherPhone->setText( addressee.phoneNumber( KABC::PhoneNumber::Car ).number() );
    mUi.fax->setText( addressee.phoneNumber( KABC::PhoneNumber::Fax ).number() );
    // Pending(michel)
    // add assistant and assistant phone
    // see if custom fields

    const KABC::Address address = addressee.address( KABC::Address::Work|KABC::Address::Pref);
    mUi.primaryAddress->setText( address.street() );
    mUi.city->setText( address.locality() );
    mUi.state->setText( address.region() );
    mUi.postalCode->setText( address.postalCode() );
    mUi.country->setText( address.country() );

    const KABC::Address other = addressee.address( KABC::Address::Home );
    mUi.otherAddress->setText( other.street() );
    mUi.otherCity->setText( other.locality() );
    mUi.otherState->setText( other.region() );
    mUi.otherPostalCode->setText( other.postalCode() );
    mUi.otherCountry->setText( other.country() );
    mUi.birthDate->setText( QDateTime( addressee.birthday() ).date().toString( QString("yyyy-MM-dd" ) ) );
    mUi.description->setPlainText( addressee.note() );
    mUi.assistant->setText( addressee.custom( "KADDRESSBOOK", "X-AssistantsName" ) );
    mUi.assistantPhone->setText( addressee.custom( "FATCRM", "X-AssistantsPhone" ) );
    mUi.leadSource->setText( addressee.custom( "FATCRM", "X-LeadSourceName" ) );
    mUi.campaign->setText( addressee.custom( "FATCRM", "X-CampaignName" ) );
    mUi.assignedTo->setText( addressee.custom( "FATCRM", "X-AssignedUserName" ) );
    mUi.reportsTo->setText( addressee.custom( "FATCRM", "X-ReportToUserName" ) );
}

void ContactDetails::clearFields ()
{
    QList<QLineEdit*> lineEdits =
        mUi.contactInformationGB->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits )
        if ( !le->text().isEmpty() ) le->clear();
    mUi.description->clear();
    mUi.firstName->setFocus();
}

void ContactDetails::disableFields()
{
    QList<QLineEdit*> lineEdits =
        mUi.contactInformationGB->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits ) {
        le->setReadOnly(true);
    }
    mUi.description->setReadOnly( true );
    mUi.saveButton->setEnabled( false );
    mCalendarButton->setEnabled( false );
    mModifyFlag = false;
}


void ContactDetails::enableFields()
{
    QList<QLineEdit*> lineEdits =
        mUi.contactInformationGB->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits ) {
        le->setReadOnly(false);
        connect( le, SIGNAL( textChanged( const QString& ) ),
                 this, SLOT( slotEnableSaving() ) );
    }
    mUi.description->setReadOnly( false );
    mCalendarButton->setEnabled( true );
    connect( mUi.description, SIGNAL( textChanged() ),
             this,  SLOT( slotEnableSaving() ) );
}

void ContactDetails::setModifyFlag()
{
    mModifyFlag = true;
}

void ContactDetails::slotEnableSaving()
{
    mUi.saveButton->setEnabled( true );
}

void ContactDetails::slotSaveContact()
{
   if ( !mContactData.empty() )
       mContactData.clear();

    QList<QLineEdit*> lineEdits =
        mUi.contactInformationGB->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits )
        mContactData[le->objectName()] = le->text();

    mContactData["description"] = mUi.description->toPlainText();

    if ( !mModifyFlag )
        emit saveContact();
    else
        emit modifyContact();
}

void ContactDetails::slotSetBirthday()
{
    mUi.birthDate->setText( mCalendarButton->calendarWidget()->selectedDate().toString( QString("yyyy-MM-dd" ) ) );

}


