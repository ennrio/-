#include "osmparser.h"

bool OSMParser::parse(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open OSM file:" << filename;
        return false;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "node") parseNode(xml);
            else if (xml.name() == "way") parseWay(xml);
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML error:" << xml.errorString();
        return false;
    }
    qDebug() << "Parsed" << nodes.size() << "nodes and" << ways.size() << "ways";
    return true;
}

const QList<OSMWay> &OSMParser::getRoadWays() const
{
    QList<OSMWay> roads;
    for (const auto &way : ways) if (way.isRoad()) roads.append(way);
    return roads;
}

void OSMParser::parseNode(QXmlStreamReader &xml)
{
    OSMNode node;
    node.lat = xml.attributes().value("lat").toDouble();
    node.lon = xml.attributes().value("lon").toDouble();

    while (!(xml.isEndElement() && xml.name() == "node")) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == "tag") {
            node.tags[xml.attributes().value("k").toString()] =
                xml.attributes().value("v").toString();
        }
    }
    nodes[xml.attributes().value("id").toLongLong()] = node;
}

void OSMParser::parseWay(QXmlStreamReader &xml)
{
    OSMWay way;
    while (!(xml.isEndElement() && xml.name() == "way")) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "nd") {
                way.nodeRefs.append(xml.attributes().value("ref").toLongLong());
            } else if (xml.name() == "tag") {
                way.tags[xml.attributes().value("k").toString()] =
                    xml.attributes().value("v").toString();
            }
        }
    }
    ways.append(way);
}
