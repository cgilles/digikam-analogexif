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

const QUrl OnlineVersionChecker::versionCheckUrl("http://analogexif.svn.sourceforge.net/viewvc/analogexif/current-version.xml");

OnlineVersionChecker::OnlineVersionChecker(QObject *parent) : QObject(parent)
{
	connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
}

void OnlineVersionChecker::checkForNewVersion()
{
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

	QNetworkRequest request(versionCheckUrl);
	QNetworkReply *reply = manager.get(request);
}

void OnlineVersionChecker::downloadFinished(QNetworkReply *reply)
{
	// mark for deletion
	reply->deleteLater();

	if(reply->error())
		return;

	if(!parser.parse(selfPlatform, reply->readAll()))
		return;

	QString readVersion = parser.getVersion();
	QDateTime readDate = parser.getDate();
	QString readDetails = parser.getDetails();

	//if(readDate > selfDate)
	//{
		emit newVersionAvailable(readVersion, readDate, readDetails);
	//}
}

VersionFileParser::VersionFileParser(QObject *parent) : QObject(parent), version(QString()), platform(QString()), details(QString()), date(QString())
{
	xmlParser = XML_ParserCreate(NULL);
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
	XML_ParserReset(xmlParser, NULL);
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
	XML_ParserReset(xmlParser, NULL);
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
		details = QString();
		date = QString();
	}

	return found;
}

void VersionFileParser::xmlStartElementHandler(void* userData, const XML_Char *name, const XML_Char **atts)
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
	else if(parser->curTag == "Date")
	{
		parser->date += xmlStr;
	}
	else if(parser->curTag == "Notes")
	{
		parser->details += xmlStr;
	}
}