#ifndef OSMPARSER_H
#define OSMPARSER_H

#include <QFile>
#include <QXmlStreamReader>
#include <QMap>
#include <QPointF>
#include <QDebug>

struct OSMNode {
    double lat;
    double lon;
    QMap<QString, QString> tags;
};

struct OSMWay {
    QList<long long> nodeRefs;
    QMap<QString, QString> tags;
    bool isRoad() const {
        return tags.contains("highway") &&
               !tags["highway"].contains("footway") &&
               !tags["highway"].contains("path");
    }
};

class OSMParser {
public:
    bool parse(const QString &filename);

    const QMap<long long, OSMNode>& getNodes() const { return nodes; }
    const QList<OSMWay>& getRoadWays() const ;

private:
    QMap<long long, OSMNode> nodes;
    QList<OSMWay> ways;

    void parseNode(QXmlStreamReader &xml);
    void parseWay(QXmlStreamReader &xml);
};

#endif // OSMPARSER_H
