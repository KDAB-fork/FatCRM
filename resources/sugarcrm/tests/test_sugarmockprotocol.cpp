/*
  This file is part of FatCRM, a desktop application for SugarCRM written by KDAB.

  Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Authors: Jeremy Entressangle <jeremy.entressangle@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QTest>
#include <QDebug>
#include "sugarmockprotocol.h"
#include "sugarjob.h"
#include "listentriesscope.h"
#include "accountshandler.h"
#include "opportunitieshandler.h"
#include "campaignshandler.h"
#include "leadshandler.h"
#include "contactshandler.h"
#include "modulename.h"

#include "sugarsession.h"

Q_DECLARE_METATYPE(Module);

class TestSugarMockProtocol : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shouldLoginCorrectly_data()
    {
        QTest::addColumn<QString>("username");
        QTest::addColumn<QString>("password");
        QTest::addColumn<int>("correctError");

        QTest::newRow("correct") << "user" << "password" << 0;
        QTest::newRow("wrong_user") << "u" << "password" << static_cast<int>(SugarJob::Errors::LoginError);
        QTest::newRow("wrong_password") << "user" << "pass" << static_cast<int>(SugarJob::Errors::LoginError);
    }

    void shouldLoginCorrectly()
    {
        //GIVEN
        QFETCH(QString, username);
        QFETCH(QString, password);
        QFETCH(int, correctError);
        SugarMockProtocol protocol;
        //WHEN
        QString sessionId, errorMessage;
        int error = protocol.login(username, password, sessionId, errorMessage);
        //THEN
        QCOMPARE(error, correctError);
        if (error == KJob::NoError) {
            QCOMPARE(sessionId, QString("1"));
        }
    }

    void serverNotFound()
    {
        //GIVEN
        SugarMockProtocol protocol;
        protocol.setServerNotFound(true);
        //WHEN
        QString sessionId, errorMessage;
        int error = protocol.login("user", "password", sessionId, errorMessage);
        //THEN
        QCOMPARE(error, static_cast<int>(SugarJob::Errors::CouldNotConnectError));
    }

    void shouldCountEntriesCorrectly_data()
    {
        QTest::addColumn<Module>("type");
        QTest::addColumn<int>("amount");

        QTest::newRow("accounts") << Module::Accounts << 3;
        QTest::newRow("opportunities") << Module::Opportunities << 2;
        QTest::newRow("leads") << Module::Leads << 1;
        QTest::newRow("campaign") << Module::Campaigns << 1;
        QTest::newRow("contact") << Module::Contacts << 1;
    }

    void shouldCountEntriesCorrectly()
    {
        QFETCH(Module, type);
        QFETCH(int, amount);
        //GIVEN
        SugarMockProtocol protocol;
        protocol.addData();
        //WHEN
        int entriesCount = 0;
        ListEntriesScope scope;
        QString query;
        QString errorMessage;
        protocol.getEntriesCount(scope, type, query, entriesCount, errorMessage);
        //THEN
        QCOMPARE(entriesCount, amount);
    }

    void ListEntriesWork_data()
    {
        QTest::addColumn<Module>("module");
        QTest::addColumn<int>("amount");

        QTest::newRow("accounts") << Module::Accounts << 3;
        QTest::newRow("opportunities") << Module::Opportunities << 2;
        QTest::newRow("leads") << Module::Leads << 1;
        QTest::newRow("campaigns") << Module::Campaigns << 1;
        QTest::newRow("contacts") << Module::Contacts << 1;
    }

    void ListEntriesWork()
    {
        QFETCH(Module, module);
        QFETCH(int, amount);
        //GIVEN
        SugarSession session(nullptr);
        SugarMockProtocol protocol;
        protocol.addData();
        AccountsHandler accountsHandler(&session);
        protocol.setAccountsHandler(&accountsHandler);
        OpportunitiesHandler opportunitiesHandler(&session);
        protocol.setOpportunitiesHandler(&opportunitiesHandler);
        CampaignsHandler campaignHandler(&session);
        protocol.setCampaignsHandler(&campaignHandler);
        LeadsHandler leadsHandler(&session);
        protocol.setLeadsHandler(&leadsHandler);
        ContactsHandler contactHandler(&session);
        protocol.setContactsHandler(&contactHandler);

        //WHEN
        ListEntriesScope scope;
        EntriesListResult result;
        QStringList selectedFields;
        QString query, orderBy, errorMessage;
        protocol.listEntries(scope, module, query, orderBy, selectedFields, result, errorMessage);
        //THEN
        QCOMPARE(result.resultCount, amount);
        QCOMPARE(result.entryList.items().size(), amount);
    }

    void getEntryShouldReturnEntry_data()
    {
        QTest::addColumn<Module>("module");
        QTest::addColumn<QString>("remoteId");
        QTest::addColumn<int>("expectedResult");

        QTest::newRow("correctAccount") << Module::Accounts << "0" << static_cast<int>(KJob::NoError);
        QTest::newRow("incorrectAccount") << Module::Accounts << "100" << static_cast<int>(SugarJob::SoapError);
        QTest::newRow("correctOpportunity") << Module::Opportunities << "100" << static_cast<int>(KJob::NoError);
        QTest::newRow("incorectOpportunity") << Module::Opportunities << "0" << static_cast<int>(SugarJob::SoapError);
    }

    void getEntryShouldReturnEntry()
    {
        QFETCH(Module, module);
        QFETCH(QString, remoteId);
        QFETCH(int, expectedResult);
        //GIVEN
        SugarSession session(nullptr);
        SugarMockProtocol protocol;
        protocol.addData();
        AccountsHandler accountsHandler(&session);
        protocol.setAccountsHandler(&accountsHandler);
        OpportunitiesHandler opportunitiesHandler(&session);
        protocol.setOpportunitiesHandler(&opportunitiesHandler);
        KDSoapGenerated::TNS__Entry_value entryValue;
        QString errorMessage;
        //WHEN
        int result = protocol.getEntry(module, remoteId, QStringList(), entryValue, errorMessage);
        //THEN
        QCOMPARE(result, expectedResult);
        if (result == 0) {
            QCOMPARE(entryValue.id(), remoteId);
        }
    }

    void setEntryShouldCorrectlySet_data()
    {
        QTest::addColumn<Module>("module");
        QTest::addColumn<QString>("id");
        QTest::addColumn<QString>("expectedId");
        QTest::addColumn<int>("expectedResult");

        QTest::newRow("addAccount") << Module::Accounts << "" << "1000" << int(KJob::NoError);
        QTest::newRow("updateAccount") << Module::Accounts << "1" << "1" << int(KJob::NoError);
        QTest::newRow("updateNonexistentAccount") << Module::Accounts << "100" << "100" << int(SugarJob::SoapError);
        QTest::newRow("addOpportunity") << Module::Opportunities << "" << "1000" << int(KJob::NoError);
        QTest::newRow("updateOpportunity") << Module::Opportunities << "100" << "100" << int(KJob::NoError);
        QTest::newRow("updateNonexistentOpportunity") << Module::Opportunities << "0" << "0" << int(SugarJob::SoapError);
    }

    void setEntryShouldCorrectlySet()
    {
        QFETCH(Module, module);
        QFETCH(QString, id);
        QFETCH(QString, expectedId);
        QFETCH(int, expectedResult);

        //GIVEN
        SugarSession session(nullptr);
        SugarMockProtocol protocol;
        protocol.addData();
        AccountsHandler accountsHandler(&session);
        protocol.setAccountsHandler(&accountsHandler);
        OpportunitiesHandler opportunitiesHandler(&session);
        protocol.setOpportunitiesHandler(&opportunitiesHandler);
        KDSoapGenerated::TNS__Name_value_list nvl;
        QList<KDSoapGenerated::TNS__Name_value> itemList;
        KDSoapGenerated::TNS__Name_value field;
        field.setName(QLatin1String("id"));
        field.setValue(id);
        itemList << field;
        nvl.setItems(itemList);
        QString errorMessage, newId;
        //WHEN
        const int result = protocol.setEntry(module, nvl, newId, errorMessage);
        //THEN
        QCOMPARE(result, expectedResult);
        if (id.isEmpty()) {
            QCOMPARE(newId, expectedId);
        }
    }

    void shouldCorrectlyListModules()
    {
        //GIVEN
        SugarSession session(nullptr);
        SugarMockProtocol *protocol = new SugarMockProtocol;
        session.setProtocol(protocol);
        protocol->setSession(&session);
        KDSoapGenerated::TNS__Select_fields selectFields;
        QString errorMessage;
        //WHEN
        protocol->listModules(selectFields,errorMessage);
        //THEN
        QStringList modules = selectFields.items();
        QCOMPARE(modules.size(), 5);
        QVERIFY(modules.contains(moduleToName(Module::Accounts)));
        QVERIFY(modules.contains(moduleToName(Module::Opportunities)));
        QVERIFY(modules.contains(moduleToName(Module::Contacts)));
        QVERIFY(modules.contains(moduleToName(Module::Documents)));
        QVERIFY(modules.contains(moduleToName(Module::Emails)));
    }
};

QTEST_MAIN(TestSugarMockProtocol)
#include "test_sugarmockprotocol.moc"
