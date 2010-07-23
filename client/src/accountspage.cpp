#include "accountspage.h"
#include "accountstreemodel.h"
#include "accountsfilterproxymodel.h"
#include "sugarclient.h"
#include "enums.h"

#include <akonadi/agentmanager.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/entitymimetypefiltermodel.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/collectionmodifyjob.h>

#include "kdcrmdata/sugaraccount.h"

#include <QMessageBox>
#include <QItemSelectionModel>
#include <QDebug>


using namespace Akonadi;

AccountsPage::AccountsPage( QWidget *parent )
    : QWidget( parent ),
      mChangeRecorder( new ChangeRecorder( this ) )
{
    mUi.setupUi( this );
    initialize();
}

AccountsPage::~AccountsPage()
{
}

void AccountsPage::slotResourceSelectionChanged( const QByteArray &identifier )
{

    if ( mAccountsCollection.isValid() ) {
        mChangeRecorder->setCollectionMonitored( mAccountsCollection, false );
    }

    /*
     * Look for the "Accounts" collection explicitly by listing all collections
     * of the currently selected resource, filtering by MIME type.
     * include statistics to get the number of items in each collection
     */
    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
    job->fetchScope().setResource( identifier );
    job->fetchScope().setContentMimeTypes( QStringList() << SugarAccount::mimeType() );
    job->fetchScope().setIncludeStatistics( true );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( slotCollectionFetchResult( KJob* ) ) );
}

void AccountsPage::slotCollectionFetchResult( KJob *job )
{

    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );

    // look for the "Accounts" collection
    Q_FOREACH( const Collection &collection, fetchJob->collections() ) {
        if ( collection.remoteId() == QLatin1String( "Accounts" ) ) {
            mAccountsCollection = collection;
            break;
        }
    }

    if ( mAccountsCollection.isValid() ) {
        mUi.newAccountPB->setEnabled( true );
        mChangeRecorder->setCollectionMonitored( mAccountsCollection, true );
        // if empty, the collection might not have been loaded yet, try synchronizing
        if ( mAccountsCollection.statistics().count() == 0 ) {
            AgentManager::self()->synchronizeCollection( mAccountsCollection );
        }

        setupCachePolicy();
    } else {
        mUi.newAccountPB->setEnabled( false );
    }
}

void AccountsPage::slotAccountChanged( const Item &item )
{
    if ( item.isValid() && item.hasPayload<SugarAccount>() ) {
        SugarClient *w = dynamic_cast<SugarClient*>( window() );
        if ( w ) {
            AccountDetails *ad = dynamic_cast<AccountDetails*>( w->detailsWidget(Account) );
            if ( ad->isEditing() ) {
                qDebug() << "ad is editing ";
                if ( !proceedIsOk() ) {
                    disconnect( mUi.accountsTV, SIGNAL( currentChanged( Akonadi::Item ) ), this, SLOT( slotAccountChanged( Akonadi::Item ) ) );
                    mUi.accountsTV->selectionModel()->setCurrentIndex( mCurrentIndex,  QItemSelectionModel::NoUpdate );
                    connect( mUi.accountsTV, SIGNAL( currentChanged( Akonadi::Item ) ), this, SLOT( slotAccountChanged( Akonadi::Item ) ) );
                    return;
                }
            }
            ad->setItem( item );
            mCurrentIndex  = mUi.accountsTV->selectionModel()->currentIndex();
            connect( ad, SIGNAL( modifyAccount() ),
                 this, SLOT( slotModifyAccount( ) ) );
        }
    }
}

void AccountsPage::slotNewAccountClicked()
{
    SugarClient *w = dynamic_cast<SugarClient*>( window() );
    if ( w ) {
        w->displayDockWidgets();
        AccountDetails* ad = dynamic_cast<AccountDetails*>( w->detailsWidget( Account ) );
        ad->clearFields();
        connect( ad, SIGNAL( saveAccount() ),
                 this, SLOT( slotAddAccount( ) ) );
        // reset
        ad->initialize();
    }
}

void AccountsPage::slotAddAccount()
{
    SugarClient *w = dynamic_cast<SugarClient*>( window() );
    AccountDetails *ad = dynamic_cast<AccountDetails*>( w->detailsWidget( Account ) );
    QMap<QString, QString> data;
    data = ad->accountData();
    SugarAccount account;
    account.setName( data.value( "name" ) );
    account.setDateEntered( data.value( "dateEntered" ) );
    account.setDateModified( data.value( "dateModified" ) );
    account.setModifiedUserId( data.value( "modifiedUserId" ) );
    account.setModifiedByName( data.value( "modifiedByName" ) );
    account.setCreatedBy( data.value( "createdBy" ) ); // id
    account.setCreatedByName( data.value( "createdByName" ) );
    account.setDescription( data.value( "description" ) );
    account.setDeleted( data.value( "deleted" ) );
    account.setAssignedUserId( data.value( "assignedUserId" ) );
    account.setAssignedUserName( data.value( "assignedUserName" ) );
    account.setAccountType( data.value( "accountType" ) );
    account.setIndustry( data.value( "industry" ) );
    account.setAnnualRevenue( data.value( "annualRevenue" ) );
    account.setPhoneFax( data.value( "phoneFax" ) );
    account.setBillingAddressStreet( data.value( "billingAddressStreet" ) );
    account.setBillingAddressCity( data.value( "billingAddressCity" ) );
    account.setBillingAddressState( data.value( "billingAddressState" ) );
    account.setBillingAddressPostalcode( data.value( "billingAddressPostalcode" ) );
    account.setBillingAddressCountry( data.value( "billingAddressCountry" ) );
    account.setRating( data.value( "rating" ) );
    account.setPhoneOffice( data.value( "phoneOffice" ) );
    account.setPhoneAlternate( data.value( "phoneAlternate" ) );
    account.setWebsite( data.value( "website" ) );
    account.setOwnership( data.value( "ownership" ) );
    account.setEmployees( data.value( "employees" ) );
    account.setTyckerSymbol( data.value( "tyckerSymbol" ) );
    account.setShippingAddressStreet( data.value( "shippingAddressStreet" ) );
    account.setShippingAddressCity( data.value( "shippingAddressCity" ) );
    account.setShippingAddressState( data.value( "shippingAddressState" ) );
    account.setShippingAddressPostalcode( data.value( "shippingAddressPostalcode" ) );
    account.setShippingAddressCountry( data.value( "shippingAddressCountry" ) );
    account.setEmail1( data.value( "email1" ) );
    account.setParentId( data.value( "parentId" ) );
    account.setParentName( data.value( "parentName" ) );
    account.setSicCode( data.value( "sicCode" ) );
    account.setCampaignId( data.value( "campaignId" ) );
    account.setCampaignName( data.value( "campaignName" ) );

    Item item;
    item.setMimeType( SugarAccount::mimeType() );
    item.setPayload<SugarAccount>( account );

    // job starts automatically
    // TODO connect to result() signal for error handling
    ItemCreateJob *job = new ItemCreateJob( item, mAccountsCollection );
    Q_UNUSED( job );
    disconnect( ad, SIGNAL( saveAccount() ),
                 this, SLOT( slotAddAccount( ) ) );
}

void AccountsPage::slotModifyAccount()
{
    const QModelIndex index = mUi.accountsTV->selectionModel()->currentIndex();
    Item item = mUi.accountsTV->model()->data( index, EntityTreeModel::ItemRole ).value<Item>();

    if ( item.isValid() ) {
        SugarAccount account;
        if ( item.hasPayload<SugarAccount>() ) {
            account = item.payload<SugarAccount>();
        }

        SugarClient *w = dynamic_cast<SugarClient*>( window() );
        AccountDetails *ad = dynamic_cast<AccountDetails*>( w->detailsWidget( Account ) );
        QMap<QString, QString> data;
        data = ad->accountData();

        account.setName( data.value( "name" ) );
        account.setDateEntered( data.value( "dateEntered" ) );
        account.setDateModified( data.value( "dateModified" ) );
        account.setModifiedUserId( data.value( "modifiedUserId" ) );
        account.setModifiedByName( data.value( "modifiedByName" ) );
        account.setCreatedBy( data.value( "createdBy" ) ); // id
        account.setCreatedByName( data.value( "createdByName" ) );
        account.setDescription( data.value( "description" ) );
        account.setDeleted( data.value( "deleted" ) );
        account.setAssignedUserId( data.value( "assignedUserId" ) );
        account.setAssignedUserName( data.value( "assignedUserName" ) );
        account.setAccountType( data.value( "accountType" ) );
        account.setIndustry( data.value( "industry" ) );
        account.setAnnualRevenue( data.value( "annualRevenue" ) );
        account.setPhoneFax( data.value( "phoneFax" ) );
        account.setBillingAddressStreet( data.value( "billingAddressStreet" ) );
        account.setBillingAddressCity( data.value( "billingAddressCity" ) );
        account.setBillingAddressState( data.value( "billingAddressState" ) );
        account.setBillingAddressPostalcode( data.value( "billingAddressPostalcode" ) );
        account.setBillingAddressCountry( data.value( "billingAddressCountry" ) );
        account.setRating( data.value( "rating" ) );
        account.setPhoneOffice( data.value( "phoneOffice" ) );
        account.setPhoneAlternate( data.value( "phoneAlternate" ) );
        account.setWebsite( data.value( "website" ) );
        account.setOwnership( data.value( "ownership" ) );
        account.setEmployees( data.value( "employees" ) );
        account.setTyckerSymbol( data.value( "tyckerSymbol" ) );
        account.setShippingAddressStreet( data.value( "shippingAddressStreet" ) );
        account.setShippingAddressCity( data.value( "shippingAddressCity" ) );
        account.setShippingAddressState( data.value( "shippingAddressState" ) );
        account.setShippingAddressPostalcode( data.value( "shippingAddressPostalcode" ) );
        account.setShippingAddressCountry( data.value( "shippingAddressCountry" ) );
        account.setEmail1( data.value( "email1" ) );
        account.setParentId( data.value( "parentId" ) );
        account.setParentName( data.value( "parentName" ) );
        account.setSicCode( data.value( "sicCode" ) );
        account.setCampaignId( data.value( "campaignId" ) );
        account.setCampaignName( data.value( "campaignName" ) );

        item.setPayload<SugarAccount>( account );

        // job starts automatically
        // TODO connect to result() signal for error handling
        ItemModifyJob *job = new ItemModifyJob( item );
        Q_UNUSED( job );
        ad->initialize();
    }
}

void AccountsPage::slotRemoveAccount()
{
    const QModelIndex index = mUi.accountsTV->selectionModel()->currentIndex();
    Item item = mUi.accountsTV->model()->data( index, EntityTreeModel::ItemRole ).value<Item>();

    removeAccountsData( item );

    if ( item.isValid() ) {
        // job starts automatically
        // TODO connect to result() signal for error handling
        ItemDeleteJob *job = new ItemDeleteJob( item );
        Q_UNUSED( job );
    }
    const QModelIndex newIndex = mUi.accountsTV->selectionModel()->currentIndex();
    if ( !index.isValid() )
        mUi.removeAccountPB->setEnabled( false );
}

void AccountsPage::slotSetCurrent( const QModelIndex& index, int start, int end )
{
        if ( start == end ) {
            QModelIndex newIdx = mUi.accountsTV->model()->index(start, 0, index);
            mUi.accountsTV->setCurrentIndex( newIdx );
        }
        //model items are loaded
        if ( mUi.accountsTV->model()->rowCount() == mAccountsCollection.statistics().count() )
            addAccountsData();
}

void AccountsPage::removeAccountsData( const Item &item )
{
    SugarClient *w = dynamic_cast<SugarClient*>( window() );
    AccountDetails *ad = dynamic_cast<AccountDetails*>( w->detailsWidget( Account ) );
    ContactDetails *cd = dynamic_cast<ContactDetails*>( w->detailsWidget( Contact ) );
    OpportunityDetails *od =dynamic_cast<OpportunityDetails*>( w->detailsWidget( Opportunity ) );

    if ( item.hasPayload<SugarAccount>() ) {
        SugarAccount account;
        account = item.payload<SugarAccount>();
        ad->removeAccountData( account.name() );
        cd->removeAccountData( account.name() );
        od->removeAccountData( account.name() );
    }
}

void AccountsPage::addAccountsData()
{
    SugarClient *w = dynamic_cast<SugarClient*>( window() );
    AccountDetails *ad = dynamic_cast<AccountDetails*>( w->detailsWidget( Account ) );
    ContactDetails *cd = dynamic_cast<ContactDetails*>( w->detailsWidget( Contact ) );
    OpportunityDetails *od =dynamic_cast<OpportunityDetails*>( w->detailsWidget( Opportunity ) );
    QModelIndex index;
    Item item;
    SugarAccount account;
    for ( int i = 0; i <  mUi.accountsTV->model()->rowCount(); ++i ) {
       index  =  mUi.accountsTV->model()->index( i, 0 );
       item = mUi.accountsTV->model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
       if ( item.hasPayload<SugarAccount>() ) {
           account = item.payload<SugarAccount>();
           ad->addAccountData( account.name(), account.id() );
           cd->addAccountData( account.name(), account.id() );
           od->addAccountData( account.name(), account.id() );
           // code below should be executed from
           // their own pages when implemented
           ad->addAssignedToData( account.assignedUserName(), account.assignedUserId() );
       }
    }
}

void AccountsPage::initialize()
{
    mUi.accountsTV->header()->setResizeMode( QHeaderView::ResizeToContents );

    connect( mUi.newAccountPB, SIGNAL( clicked() ),
             this, SLOT( slotNewAccountClicked() ) );
    connect( mUi.removeAccountPB, SIGNAL( clicked() ),
             this, SLOT( slotRemoveAccount() ) );

    // automatically get the full data when items change
    mChangeRecorder->itemFetchScope().fetchFullPayload( true );

    AccountsTreeModel *accountsModel = new AccountsTreeModel( mChangeRecorder, this );

    AccountsTreeModel::Columns columns;
    columns << AccountsTreeModel::Name
            << AccountsTreeModel::City
            << AccountsTreeModel::Country
            << AccountsTreeModel::Phone
            << AccountsTreeModel::Email
            << AccountsTreeModel::CreatedBy;
    accountsModel->setColumns( columns );

    // same as for the ContactsTreeModel, not strictly necessary
    EntityMimeTypeFilterModel *filterModel = new EntityMimeTypeFilterModel( this );
    filterModel->setSourceModel( accountsModel );
    filterModel->addMimeTypeInclusionFilter( SugarAccount::mimeType() );
    filterModel->setHeaderGroup( EntityTreeModel::ItemListHeaders );

    AccountsFilterProxyModel *filter = new AccountsFilterProxyModel( this );
    filter->setSourceModel( filterModel );
    mUi.accountsTV->setModel( filter );

    connect( mUi.searchLE, SIGNAL( textChanged( const QString& ) ),
             filter, SLOT( setFilterString( const QString& ) ) );

    connect( mUi.accountsTV, SIGNAL( currentChanged( Akonadi::Item ) ), this, SLOT( slotAccountChanged( Akonadi::Item ) ) );


    connect( mUi.accountsTV->model(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), SLOT( slotSetCurrent( const QModelIndex&,int,int ) ) );

    connect( mUi.accountsTV->model(), SIGNAL( dataChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( slotUpdateItemDetails( const QModelIndex&, const QModelIndex& ) ) );
}

void AccountsPage::syncronize()
{
    AgentManager::self()->synchronizeCollection( mAccountsCollection );
}

void AccountsPage::cachePolicyJobCompleted( KJob* job)
{
    if ( job->error() )
        emit statusMessage( tr("Error when setting cachepolicy: %1").arg( job->errorString() ) );
    else
        emit statusMessage( tr("Cache policy set") );

}

void AccountsPage::setupCachePolicy()
{
    CachePolicy policy;
    policy.setIntervalCheckTime( 1 ); // Check for new data every minute
    policy.setInheritFromParent( false );
    mAccountsCollection.setCachePolicy( policy );
    CollectionModifyJob *job = new CollectionModifyJob( mAccountsCollection );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( cachePolicyJobCompleted( KJob* ) ) );
}

void AccountsPage::slotUpdateItemDetails( const QModelIndex& topLeft, const QModelIndex& bottomRight )
{
    Q_UNUSED( bottomRight );
    Item item;
    SugarAccount account;
    item = mUi.accountsTV->model()->data( topLeft, EntityTreeModel::ItemRole ).value<Item>();
    slotAccountChanged( item );
}

bool AccountsPage::proceedIsOk()
{
    bool proceed = true;
    QMessageBox msgBox;
    msgBox.setText( tr( "The current item has been modified." ) );
    msgBox.setInformativeText( tr( "Do you want to save your changes?" ) );
    msgBox.setStandardButtons( QMessageBox::Save |
                               QMessageBox::Discard );
    msgBox.setDefaultButton( QMessageBox::Save );
    int ret = msgBox.exec();
    if ( ret == QMessageBox::Save )
        proceed = false;
    return proceed;
}
