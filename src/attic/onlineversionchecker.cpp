/*
    Copyright (C) 2010 C-41 Bytes <contact@c41bytes.com>

    This file is part of AnalogExif.

    AnalogExif is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AnalogExif is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AnalogExif.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "onlineversionchecker.h"

#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QSettings>

const QUrl OnlineVersionChecker::versionCheckUrl("http://analogexif.svn.sourceforge.net/viewvc/analogexif/current-version.xml");
const QUrl OnlineVersionChecker::downloadUrl("http://sourceforge.net/projects/analogexif/files/");

OnlineVersionChecker::OnlineVersionChecker(QObject *parent) : QObject(parent), curRequest(nullptr)
{
    connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
}

bool OnlineVersionChecker::needToCheckVersion()
{
    QSettings settings;

    int checkInterval = settings.value("CheckForUpdatePeriod", 0).toInt();

    if(!checkInterval)
        return false;

    QDateTime lastCheck = settings.value("LastCheckForUpdate", QDateTime()).toDateTime();

    if(lastCheck.isValid())
    {
        int daysBetween = lastCheck.daysTo(QDateTime::currentDateTime());

        switch(checkInterval)
        {
        case 1: // every day
            if(daysBetween < 1)
                return false;
            break;
        case 2: // every week
            if(daysBetween < 7)
                return false;
            break;
        case 3: // every month
            if(daysBetween < 31)
                return false;
            break;
        default:
            return false;
        }
    }

    return true;
}

void OnlineVersionChecker::checkForNewVersion(bool force)
{
    QSettings settings;

    if(!force && !needToCheckVersion())
        return;

    settings.setValue("LastCheckForUpdate", QDateTime::currentDateTime());
    settings.sync();

    // parse self version
    QFile file(":/version.xml");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    if(!parser.parse(in.readAll()))
        return;

    // get self version and date
    selfVersion = parser.getVersion();
    selfPlatform = parser.getPlatform();
    selfDate = parser.getDate();
    selfReleaseNumber = parser.getReleaseNumber();

    QNetworkRequest request(versionCheckUrl);
    curRequest = manager.get(request);

    if(curRequest->error())
        emit newVersionCheckError(curRequest->error());
}

void OnlineVersionChecker::cancelCheck()
{
    if(curRequest)
        curRequest->abort();
}

void OnlineVersionChecker::downloadFinished(QNetworkReply *reply)
{
    // mark for deletion
    reply->deleteLater();
    curRequest = nullptr;

    QNetworkReply::NetworkError error = reply->error();
    if(error)
    {
        emit newVersionCheckError(error);
        return;
    }

    if(!parser.parse(selfPlatform, reply->readAll()))
    {
        emit newVersionCheckError(QNetworkReply::ProtocolFailure);
        return;
    }

    QString readVersion = parser.getVersion();
    QDateTime readDate = parser.getDate();
    QString readDetails = parser.getDetails();
    int readReleaseNumber = parser.getReleaseNumber();

    if(readReleaseNumber > selfReleaseNumber)
    {
        emit newVersionAvailable(selfVersion, readVersion, readDate, readDetails);
    }
    else
    {
        emit newVersionCheckError(QNetworkReply::NoError);
    }
}

void OnlineVersionChecker::error(QNetworkReply::NetworkError code)
{
    emit newVersionCheckError(code);
}


void OnlineVersionChecker::openDownloadPage()
{
    QDesktopServices::openUrl(downloadUrl);
}

VersionFileParser::VersionFileParser(QObject *parent) : QObject(parent), version(QString()), releaseNumber(QString()), platform(QString()), details(QString()), date(QString())
{
    xmlParser = XML_ParserCreate(nullptr);
}

VersionFileParser::~VersionFileParser()
{
    if(xmlParser)
        XML_ParserFree(xmlParser);
}

// parse single-record XML file
bool VersionFileParser::parse(const QString& xmlData)
{
    // sanity check
    if(!xmlParser)
        return false;

    // reset and restart parser
    XML_ParserReset(xmlParser, nullptr);
    // set element start/end handlers
    XML_SetElementHandler(xmlParser,
        (XML_StartElementHandler)&VersionFileParser::xmlStartElementHandler,
        (XML_EndElementHandler)&VersionFileParser::xmlEndElementHandler);

    // set element data handler
    XML_SetCharacterDataHandler(xmlParser, (XML_CharacterDataHandler)&VersionFileParser::xmlCharacterDataHandler);

    // set pointer to the current object
    XML_SetUserData(xmlParser, this);

    // reset temp variables
    version = QString();
    platform = QString();
    details = QString();
    date = QString();
    lookupPlatform = QString();
    releaseNumber = QString();
    found = false;

    // parse the XML
    QByteArray xml = xmlData.toUtf8();
    if(!XML_Parse(xmlParser, xml.constData(), xml.size(), true))
        return false;

    return true;
}

// parse XML file and lookup for the specific version
bool VersionFileParser::parse(const QString& ver, const QString& xmlData)
{
    // sanity check
    if(!xmlParser)
        return false;
    
    // reset and restart parser
    XML_ParserReset(xmlParser, nullptr);
    // set element start/end handlers
    XML_SetElementHandler(xmlParser,
        (XML_StartElementHandler)&VersionFileParser::xmlStartElementHandler,
        (XML_EndElementHandler)&VersionFileParser::xmlEndElementHandler);

    // set element data handler
    XML_SetCharacterDataHandler(xmlParser, (XML_CharacterDataHandler)&VersionFileParser::xmlCharacterDataHandler);

    // set pointer to the current object
    XML_SetUserData(xmlParser, this);

    // reset temp variables
    version = QString();
    platform = QString();
    details = QString();
    date = QString();
    releaseNumber = QString();
    lookupPlatform = ver;
    found = false;

    // parse the XML
    QByteArray xml = xmlData.toUtf8();
    if(!XML_Parse(xmlParser, xml.constData(), xml.size(), true))
    {
        if(XML_GetErrorCode(xmlParser) != XML_ERROR_ABORTED)
            return false;
    }

    // if not found - reset all values
    if(!found)
    {
        version = QString();
        platform = QString();
        releaseNumber = QString();
        details = QString();
        date = QString();
    }

    return found;
}

void VersionFileParser::xmlStartElementHandler(void* userData, const XML_Char *name, const XML_Char **)
{
    VersionFileParser* parser = static_cast<VersionFileParser*>(userData);

    parser->curTag = QString::fromUtf8(name);
}

void VersionFileParser::xmlEndElementHandler(void* userData, const XML_Char *name)
{
    VersionFileParser* parser = static_cast<VersionFileParser*>(userData);

    if(QString::fromUtf8(name) == "Release")
    {
        if(!parser->lookupPlatform.isNull())
        {
            if(parser->lookupPlatform == parser->platform)
            {
                // found required version
                parser->found = true;

                // stop parsing
                XML_StopParser(parser->xmlParser, XML_FALSE);
            }
            else
            {
                // reset all values
                parser->version = QString();
                parser->platform = QString();
                parser->details = QString();
                parser->releaseNumber = QString();
                parser->date = QString();
            }
        }
    }

    parser->curTag = "";
}

void VersionFileParser::xmlCharacterDataHandler(void *userData, const XML_Char *s, int len)
{
    VersionFileParser* parser = static_cast<VersionFileParser*>(userData);

    // ignore all consequent data if requested version
    if(parser->found)
        return;

    QString xmlStr = QString::fromUtf8(s, len);

    if(parser->curTag == "OS")
    {
        parser->platform += xmlStr;
    }
    else if(parser->curTag == "Version")
    {
        parser->version += xmlStr;
    }
    else if(parser->curTag == "ReleaseNumber")
    {
        parser->releaseNumber += xmlStr;
    }
    else if(parser->curTag == "Date")
    {
        parser->date += xmlStr;
    }
    else if(parser->curTag == "Notes")
    {
        parser->details += xmlStr;
    }
}
