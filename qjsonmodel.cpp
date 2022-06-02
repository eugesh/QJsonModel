/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2011 SCHUTZ Sacha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include "qjsonmodel.h"
#include "serialization.h"
#include <QFile>
#include <QDebug>
#include <QFont>
#include <string>


bool contains(const QStringList& list, const QString &value) {
    for (auto val : list) {
        if (value.contains(val, Qt::CaseInsensitive))
            return true;
    }

    return false;
}

QJsonTreeItem::QJsonTreeItem(QJsonTreeItem *parent)
{
    mParent = parent;
}

QJsonTreeItem::~QJsonTreeItem()
{
    qDeleteAll(mChilds);
}

void QJsonTreeItem::appendChild(QJsonTreeItem *item)
{
    mChilds.append(item);
}

QJsonTreeItem *QJsonTreeItem::child(int row)
{
    return mChilds.value(row);
}

QJsonTreeItem *QJsonTreeItem::parent()
{
    return mParent;
}

int QJsonTreeItem::childCount() const
{
    return mChilds.count();
}

int QJsonTreeItem::row() const
{
    if (mParent)
        return mParent->mChilds.indexOf(const_cast<QJsonTreeItem*>(this));

    return 0;
}

void QJsonTreeItem::setKey(const QString &key)
{
    mKey = key;
}

void QJsonTreeItem::setValue(const QVariant &value)
{
    mValue = value;
}

void QJsonTreeItem::setFieldType(const JsonFieldType &type) {
    mFieldType = type;
    mAttrMap.insert(tagNames[TYPE], type);
}

void QJsonTreeItem::setAddress(int addr)
{
    mAddress = addr;
    mAttrMap.insert(tagNames[ADDR], addr);
}

void QJsonTreeItem::setSize(int size)
{
    mLength = size;
    mAttrMap.insert(tagNames[SIZE], size);
}

void QJsonTreeItem::setType(const QJsonValue::Type &type)
{
    mType = type;
}

void QJsonTreeItem::setDescription(const QString &desc)
{
    mDescription = desc;
    mAttrMap.insert(tagNames[DESC], desc);
}

void QJsonTreeItem::setEditMode(const JsonEditMode &editMode)
{
    mEditMode = editMode;
}

QString QJsonTreeItem::key() const
{
    return mKey;
}

QVariant QJsonTreeItem::value() const
{
    return mValue;
}

QString QJsonTreeItem::description() const
{
    return mDescription;
}

QJsonTreeItem::JsonEditMode QJsonTreeItem::editMode() const
{
    return mEditMode;
}

QJsonTreeItem::JsonFieldType QJsonTreeItem::fieldType() const
{
    return mFieldType;
}

int QJsonTreeItem::address() const
{
    return mAddress;
}

int QJsonTreeItem::size() const
{
    return mLength;
}

QJsonValue::Type QJsonTreeItem::type() const
{
    return mType;
}

QJsonTreeItem* QJsonTreeItem::load(const QJsonValue& value, const QStringList &exceptions, QJsonTreeItem* parent)
{
    QJsonTreeItem * rootItem = new QJsonTreeItem(parent);
    rootItem->setKey("root");

    if (value.isObject()) {
        //Get all QJsonValue childs
        auto keys = value.toObject().keys();
        for (const QString &key : qAsConst(keys)) {
            if (contains(exceptions, key)) {
                continue;
            }
            QJsonValue v = value.toObject().value(key);
            QJsonTreeItem * child = load(v, exceptions, rootItem);
            child->setKey(key);
            child->setType(v.type());
            rootItem->appendChild(child);
        }
    } else if (value.isArray()) {
        //Get all QJsonValue childs
        int index = 0;
        auto arr = value.toArray();
        for (const QJsonValue &v : arr) {
            QJsonTreeItem * child = load(v, exceptions, rootItem);
            child->setKey(QString::number(index));
            child->setType(v.type());
            rootItem->appendChild(child);
            ++index;
        }
    } else {
        rootItem->setValue(value.toVariant());
        rootItem->setType(value.type());
    }

    return rootItem;
}

QJsonTreeItem* QJsonTreeItem::loadWithDesc(QMap<QString, QVariant> &fieldValueMap, QMap<int, QString> &structureMap, QMap<QString, QMap<QString, QVariant> > &passDescriptionMap,
                                           const QJsonValue& value, const QJsonValue& description, const QStringList &exceptions, QJsonTreeItem * parent)
{
    QJsonTreeItem * rootItem = new QJsonTreeItem(parent);
    rootItem->setKey("root");

    if (value.isObject()) {
        //Get all QJsonValue childs
        auto keys = value.toObject().keys();
        for (const QString &key : qAsConst(keys)) {
            if (contains(exceptions, key)) {
                continue;
            }
            QJsonValue v = value.toObject().value(key);
            QJsonValue d = description.toObject().value(key);
            QJsonTreeItem * child = loadWithDesc(fieldValueMap, structureMap, passDescriptionMap, v, d, exceptions, rootItem);
            child->setKey(key);
            child->setType(v.type());
            rootItem->appendChild(child);

            if (!child->attributeMap().isEmpty()) {
                structureMap.insert(child->address(), key);
                passDescriptionMap.insert(key, child->attributeMap());
                fieldValueMap.insert(key, child->value());
            }
        }
    } else if (value.isArray()) {
        //Get all QJsonValue childs
        int index = 0;
        for (int i = 0; i < value.toArray().count(); ++i) {
            QJsonValue v = value.toArray()[i];
            QJsonValue d = description.toArray()[i];
            QJsonTreeItem * child = loadWithDesc(fieldValueMap, structureMap, passDescriptionMap, v, d, exceptions, rootItem);
            child->setKey(QString::number(index));
            child->setType(v.type());
            rootItem->appendChild(child);
            ++index;
        }
    } else {
        rootItem->setValue(value.toVariant());
        rootItem->setType(value.type());
        auto modeStr = description.toVariant().toMap()["mode2"].toString();
        QJsonTreeItem::JsonEditMode mode = ! modeStr.contains("r", Qt::CaseInsensitive) ? QJsonTreeItem::W :
                                           modeStr.contains("w", Qt::CaseInsensitive) ? QJsonTreeItem::RW : QJsonTreeItem::R;
        rootItem->setEditMode(mode);

        rootItem->setFieldType(fromString(description.toVariant().toMap()["type"].toString()));
        bool isOk;
        rootItem->setAddress(description.toVariant().toMap()["addr"].toString().toInt(&isOk, 16));
        rootItem->setSize(description.toVariant().toMap()["size"].toInt(&isOk));
        rootItem->setDescription(description.toVariant().toMap()["desc"].toString());
        // structureMap.insert(rootItem->address(), rootItem->key());
        // passDescriptionMap.insert(rootItem->key(), rootItem->attributeMap());
        // fieldValueMap.insert(rootItem->key(), rootItem->value());
    }

    return rootItem;
}

QJsonTreeItem::JsonFieldType QJsonTreeItem::fromString(const QString &str)
{
    if (str.contains("uint", Qt::CaseInsensitive)) {
        return UINT;
    } else if (str.contains("int", Qt::CaseInsensitive)) {
        return INT;
    } else if (str.contains("float", Qt::CaseInsensitive)) {
        return FLOAT;
    } else if (str.contains("str", Qt::CaseInsensitive)) {
        return STRING;
    } else if (str.contains("date", Qt::CaseInsensitive)) {
        return DATE;
    }
}

QMap<QString, QVariant> QJsonTreeItem::attributeMap() const
{
    return mAttrMap;
}

//=========================================================================

inline uchar hexdig(uint u)
{
    return (u < 0xa ? '0' + u : 'a' + u - 0xa);
}

QByteArray escapedString(const QString &s)
{
    QByteArray ba(s.length(), Qt::Uninitialized);
    uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(ba.constData()));
    const uchar *ba_end = cursor + ba.length();
    const ushort *src = reinterpret_cast<const ushort *>(s.constBegin());
    const ushort *const end = reinterpret_cast<const ushort *>(s.constEnd());
    while (src != end) {
        if (cursor >= ba_end - 6) {
            // ensure we have enough space
            int pos = cursor - (const uchar *)ba.constData();
            ba.resize(ba.size() * 2);
            cursor = (uchar *)ba.data() + pos;
            ba_end = (const uchar *)ba.constData() + ba.length();
        }
        uint u = *src++;
        if (u < 0x80) {
            if (u < 0x20 || u == 0x22 || u == 0x5c) {
                *cursor++ = '\\';
                switch (u) {
                case 0x22:
                    *cursor++ = '"';
                    break;
                case 0x5c:
                    *cursor++ = '\\';
                    break;
                case 0x8:
                    *cursor++ = 'b';
                    break;
                case 0xc:
                    *cursor++ = 'f';
                    break;
                case 0xa:
                    *cursor++ = 'n';
                    break;
                case 0xd:
                    *cursor++ = 'r';
                    break;
                case 0x9:
                    *cursor++ = 't';
                    break;
                default:
                    *cursor++ = 'u';
                    *cursor++ = '0';
                    *cursor++ = '0';
                    *cursor++ = hexdig(u >> 4);
                    *cursor++ = hexdig(u & 0xf);
                }
            } else {
                *cursor++ = (uchar)u;
            }
        } else if (QUtf8Functions::toUtf8<QUtf8BaseTraits>(u, cursor, src, end) < 0) {
            // failed to get valid utf8 use JSON escape sequence
            *cursor++ = '\\';
            *cursor++ = 'u';
            *cursor++ = hexdig(u >> 12 & 0x0f);
            *cursor++ = hexdig(u >> 8 & 0x0f);
            *cursor++ = hexdig(u >> 4 & 0x0f);
            *cursor++ = hexdig(u & 0x0f);
        }
    }
    ba.resize(cursor - (const uchar *)ba.constData());
    return ba;
}

QJsonModel::QJsonModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem{new QJsonTreeItem}
    , mRootDescriptionItem{new QJsonTreeItem}
{
    mHeaders.append("key");
    mHeaders.append("value");
}

QJsonModel::QJsonModel(const QString& fileName, const QString &fileNameDesc, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem{new QJsonTreeItem}
{
    mHeaders.append("key");
    mHeaders.append("value");

    if (fileNameDesc.isEmpty())
        load(fileName);
    else
        load(fileName, fileNameDesc);
}

QJsonModel::QJsonModel(QIODevice * device, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem{new QJsonTreeItem}
{
    mHeaders.append("key");
    mHeaders.append("value");
    load(device);
}

QJsonModel::QJsonModel(const QByteArray& json, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem{new QJsonTreeItem}
{
    mHeaders.append("key");
    mHeaders.append("value");
    loadJson(json);
}

QJsonModel::~QJsonModel()
{
    delete mRootItem;
}

bool QJsonModel::load(const QString &fileName)
{
    QFile file(fileName);
    bool success = false;
    if (file.open(QIODevice::ReadOnly)) {
        success = load(&file);
        file.close();
    }
    else success = false;

    return success;
}

bool QJsonModel::load(const QString& fileName, const QString descFileName)
{
    QFile file(fileName), descFile(descFileName);
    bool success = false;
    if (file.open(QIODevice::ReadOnly) && descFile.open(QIODevice::ReadOnly)) {
        success = load(&file, &descFile);
        file.close();
    } else {
        success = false;
    }

    return success;
}

bool QJsonModel::load(QIODevice *device)
{
    return loadJson(device->readAll());
}

bool QJsonModel::load(QIODevice * device, QIODevice * deviceDesc)
{
    return loadJson(device->readAll(), deviceDesc->readAll());
}

bool QJsonModel::loadJson(const QByteArray &json)
{
    auto const& jdoc = QJsonDocument::fromJson(json);

    if (!jdoc.isNull()) {
        beginResetModel();
        delete mRootItem;
        if (jdoc.isArray()) {
            mRootItem = QJsonTreeItem::load(QJsonValue(jdoc.array()), mExceptions);
            mRootItem->setType(QJsonValue::Array);
        } else {
            mRootItem = QJsonTreeItem::load(QJsonValue(jdoc.object()), mExceptions);
            mRootItem->setType(QJsonValue::Object);
        }
        endResetModel();
        return true;
    }

    qDebug()<<Q_FUNC_INFO<<"cannot load json";
    return false;
}

bool QJsonModel::loadJson(const QByteArray& json, const QByteArray& descJson)
{
    auto const& jdoc = QJsonDocument::fromJson(json);
    auto const& jdocDesc = QJsonDocument::fromJson(descJson);

    if (!jdoc.isNull()) {
        beginResetModel();
        delete mRootItem;
        if (jdoc.isArray()) {
            mRootItem = QJsonTreeItem::loadWithDesc(mFieldValueMap, mStructureMap, mPassDescriptionMap, QJsonValue(jdoc.array()), QJsonValue(jdocDesc.array()), mExceptions);
            mRootItem->setType(QJsonValue::Array);
        } else {
            mRootItem = QJsonTreeItem::loadWithDesc(mFieldValueMap, mStructureMap, mPassDescriptionMap, QJsonValue(jdoc.object()), QJsonValue(jdocDesc.object()), mExceptions);
            mRootItem->setType(QJsonValue::Object);
            // mStructureMap.insert(mRootItem->address(), mRootItem->key());
            // mPassDescriptionMap.insert();
        }
        endResetModel();
        return true;
    }

    qDebug()<<Q_FUNC_INFO<<"cannot load json";
    return false;
}

QVariant QJsonModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QJsonTreeItem *item = static_cast<QJsonTreeItem*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        if (index.column() == 0)
            return QString("%1").arg(item->key());

        if (index.column() == 1)
            return item->value();
    } else if (Qt::EditRole == role) {
        //if (item->editMode() == QJsonTreeItem::R)
          //  return QVariant();

        if (index.column() == 1) {
            return item->value();
        }
    } else if (Qt::ToolTipRole == role) {
        return item->description();
    }

    return QVariant();
}

bool QJsonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int col = index.column();
    if (Qt::EditRole == role) {
        if (col == 1) {
            QJsonTreeItem *item = static_cast<QJsonTreeItem*>(index.internalPointer());
            if (item->editMode() == QJsonTreeItem::R)
                return false;

            item->setValue(value);
            emit dataChanged(index, index, {Qt::EditRole});
            return true;
        }
    }

    return false;
}

QVariant QJsonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        return mHeaders.value(section);
    } else
        return QVariant();
}

QModelIndex QJsonModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QJsonTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = mRootItem;
    else
        parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());

    QJsonTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex QJsonModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QJsonTreeItem *childItem = static_cast<QJsonTreeItem*>(index.internalPointer());
    QJsonTreeItem *parentItem = childItem->parent();

    if (parentItem == mRootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int QJsonModel::rowCount(const QModelIndex &parent) const
{
    QJsonTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = mRootItem;
    else
        parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int QJsonModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

Qt::ItemFlags QJsonModel::flags(const QModelIndex &index) const
{
    int col   = index.column();
    auto item = static_cast<QJsonTreeItem*>(index.internalPointer());

    auto isArray = QJsonValue::Array == item->type();
    auto isObject = QJsonValue::Object == item->type();

    if ((col == 1) && !(isArray || isObject)) {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    } else {
        return QAbstractItemModel::flags(index);
    }
}

QByteArray QJsonModel::json()
{
    auto jsonValue = genJson(mRootItem);
    QByteArray json;
    if (jsonValue.isNull()) {
        return json;
    }
    if (jsonValue.isArray()) {
        arrayToJson(jsonValue.toArray(), json, 0, false);
    } else {
        objectToJson(jsonValue.toObject(), json, 0, false);
    }
    return json;
}

void QJsonModel::objectToJson(QJsonObject jsonObject, QByteArray &json, int indent, bool compact)
{
    json += compact ? "{" : "{\n";
    objectContentToJson(jsonObject, json, indent + (compact ? 0 : 1), compact);
    json += QByteArray(4 * indent, ' ');
    json += compact ? "}" : "}\n";
}
void QJsonModel::arrayToJson(QJsonArray jsonArray, QByteArray &json, int indent, bool compact)
{
    json += compact ? "[" : "[\n";
    arrayContentToJson(jsonArray, json, indent + (compact ? 0 : 1), compact);
    json += QByteArray(4 * indent, ' ');
    json += compact ? "]" : "]\n";
}

void QJsonModel::arrayContentToJson(QJsonArray jsonArray, QByteArray &json, int indent, bool compact)
{
    if (jsonArray.size() <= 0) {
        return;
    }
    QByteArray indentString(4 * indent, ' ');
    int i = 0;
    while (1) {
        json += indentString;
        valueToJson(jsonArray.at(i), json, indent, compact);
        if (++i == jsonArray.size()) {
            if (!compact)
                json += '\n';
            break;
        }
        json += compact ? "," : ",\n";
    }
}
void QJsonModel::objectContentToJson(QJsonObject jsonObject, QByteArray &json, int indent, bool compact)
{
    if (jsonObject.size() <= 0) {
        return;
    }
    QByteArray indentString(4 * indent, ' ');
    int i = 0;
    while (1) {
        QString key = jsonObject.keys().at(i);
        json += indentString;
        json += '"';
        json += escapedString(key);
        json += compact ? "\":" : "\": ";
        valueToJson(jsonObject.value(key), json, indent, compact);
        if (++i == jsonObject.size()) {
            if (!compact)
                json += '\n';
            break;
        }
        json += compact ? "," : ",\n";
    }
}

void QJsonModel::valueToJson(QJsonValue jsonValue, QByteArray &json, int indent, bool compact)
{
    QJsonValue::Type type = jsonValue.type();
    switch (type) {
    case QJsonValue::Bool:
        json += jsonValue.toBool() ? "true" : "false";
        break;
    case QJsonValue::Double: {
        const double d = jsonValue.toDouble();
        if (qIsFinite(d)) {
            json += QByteArray::number(d, 'f', QLocale::FloatingPointShortest);
        } else {
            json += "null"; // +INF || -INF || NaN (see RFC4627#section2.4)
        }
        break;
    }
    case QJsonValue::String:
        json += '"';
        json += escapedString(jsonValue.toString());
        json += '"';
        break;
    case QJsonValue::Array:
        json += compact ? "[" : "[\n";
        arrayContentToJson(jsonValue.toArray(), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4 * indent, ' ');
        json += ']';
        break;
    case QJsonValue::Object:
        json += compact ? "{" : "{\n";
        objectContentToJson(jsonValue.toObject(), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4 * indent, ' ');
        json += '}';
        break;
    case QJsonValue::Null:
    default:
        json += "null";
    }
}

void QJsonModel::addException(const QStringList &exceptions)
{
    mExceptions = exceptions;
}

// Experimental
int64_t QJsonModel::serialize(unsigned char* arr) const
{
    int64_t totalLen = 0;
    int shift = 0;

    int lastAddr = mStructureMap.keys().last();
    const QString lastField = mStructureMap[lastAddr];
    int lastFieldLen = mPassDescriptionMap[lastField][tagNames[SIZE]].toInt();

    totalLen = lastAddr + lastFieldLen;

    arr = new unsigned char[totalLen];

    for (auto key : mStructureMap.keys()) {
        const QString field = mStructureMap[key];
        int address = mPassDescriptionMap[field][tagNames[ADDR]].toInt();
        int len = mPassDescriptionMap[field][tagNames[SIZE]].toInt();
        QJsonTreeItem::JsonFieldType fieldType = static_cast<QJsonTreeItem::JsonFieldType>(mPassDescriptionMap[field][tagNames[TYPE]].toInt());
        QVariant value = mFieldValueMap[field];

        // unsigned char *tmp = new unsigned char[len];
        // unsigned char *tmp = static_cast<unsigned char*>(calloc(sizeof(unsigned char), len));
        unsigned char tmp[1024] = {0};
        switch(fieldType) {
        case QJsonTreeItem::STRING:
            strncpy(reinterpret_cast<char*>(tmp), value.toString().toStdString().c_str(), len);
            break;
        case QJsonTreeItem::INT: {
            // sprintf(tmp, "%d", value.toInt());
            // itoa (i,buffer,10);
            // itoa(value.toInt(), tmp, 16);
            // tmp = reinterpret_cast<unsigned char*>(value.toInt());
            // int val = value.toInt();
            // memcpy(tmp, reinterpret_cast<unsigned char*> (&val), 4);
            int val = value.toInt();
            szn::intToBytes(tmp, val);
        } break;
        case QJsonTreeItem::UINT: {
            // sprintf(tmp, "%ud", value.toUInt());
            // tmp = reinterpret_cast<unsigned char*>(value.toUInt());
            // unsigned int val = value.toUInt();
            // memcpy(tmp, reinterpret_cast<unsigned char*> (&val), 4);
            unsigned int val = value.toUInt();
            szn::intToBytes(tmp, val);
        } break;
        case QJsonTreeItem::FLOAT: {
            if (len == 4) {
                float val = value.toFloat();
                szn::floatToBytes(tmp, val);
                // float val = value.toFloat();
                // memcpy(tmp, reinterpret_cast<unsigned char*> (&val), 4);
                // sprintf(tmp, "%f", value.toFloat());
            } else if (len == 8) {
                double val = value.toDouble();
                szn::floatToBytes(tmp, val);
                // memcpy(tmp, reinterpret_cast<unsigned char*> (&val), 8);
                // sprintf(tmp, "%f", value.toDouble());
            }
        } break;
        case QJsonTreeItem::DATE:
            break;
        }

        snprintf(reinterpret_cast<char*>(arr) + shift, totalLen - shift, "%s", reinterpret_cast<char*>(tmp));
        // delete []tmp;
        // free(tmp);
        shift += len;
    }

    std::cout << arr << std::endl;

    return totalLen;
}

/**
 * @brief QJsonModel::serialize
 * Represents JSON as char array relative to the address in the description
 * @return serialized bytes sequence
 */
QByteArray QJsonModel::serialize() const
{
    QByteArray arr;

    for (auto key : mStructureMap.keys()) {
        const QString field = mStructureMap[key];
        int address = mPassDescriptionMap[field][tagNames[ADDR]].toInt();
        int len = mPassDescriptionMap[field][tagNames[SIZE]].toInt();
        QJsonTreeItem::JsonFieldType fieldType = static_cast<QJsonTreeItem::JsonFieldType>(mPassDescriptionMap[field][tagNames[TYPE]].toInt());
        QVariant value = mFieldValueMap[field];

        QByteArray tmp;
        tmp.resize(len);
        switch(fieldType) {
        case QJsonTreeItem::STRING:
            // tmp = value.toString().toLatin1();// toUtf8();
            tmp = QByteArray::fromStdString(value.toString().toStdString());
            break;
        case QJsonTreeItem::INT:
        case QJsonTreeItem::UINT: {
                int val = value.toInt();
                szn::intToBytes(reinterpret_cast<unsigned char*>(tmp.data()), val);
            }
            break;
        case QJsonTreeItem::FLOAT: {
                float val = value.toFloat();
                szn::floatToBytes(reinterpret_cast<unsigned char*>(tmp.data()), val);
            }
            break;
        case QJsonTreeItem::DOUBLE: {
                double val = value.toDouble();
                szn::floatToBytes(reinterpret_cast<unsigned char*>(tmp.data()), val);
            }
            break;
        case QJsonTreeItem::DATE:
            // Not implemented yet
            break;
        }

        arr.push_back(tmp);
    }

#ifdef QT_DEBUG
    qDebug() << arr;
    qDebug() << QString::fromLatin1(arr).toUtf8();
    std::cout << QString::fromLatin1(arr).toUtf8().toStdString() << std::endl;
    std::cout << arr.data() << std::endl;
    std::cout << "arr size = " << arr.size() << std::endl;

    for (int i = 0; i < arr.size(); ++i) {
        int16_t val = int16_t(arr[i]) >= 0 ? int16_t(arr[i]) : (256 + int16_t(arr[i]));
        std::cout << val << '\t';
    }
    std::cout << '\n';

    for (int i = 0; i < arr.size(); ++i) {
        szn::print(arr[i]);
        std::cout << '\t';
    }
    std::cout << '\n';
#endif

    return arr;
}

/*QByteArray QJsonModel::serialize() const
{
    QByteArray arr;

    for (auto key : mStructureMap.keys()) {
        const QString field = mStructureMap[key];
        int address = mPassDescriptionMap[field][tagNames[ADDR]].toInt();
        int len = mPassDescriptionMap[field][tagNames[SIZE]].toInt();
        QJsonTreeItem::JsonFieldType fieldType = static_cast<QJsonTreeItem::JsonFieldType>(mPassDescriptionMap[field][tagNames[TYPE]].toInt());
        QVariant value = mFieldValueMap[field];

        QByteArray tmp;
        switch(fieldType) {
        case QJsonTreeItem::STRING:
            // tmp = value.toString().toLatin1();// toUtf8();
            tmp = QByteArray::fromStdString(value.toString().toStdString());
            break;
        case QJsonTreeItem::INT:
        case QJsonTreeItem::UINT:
            tmp = QByteArray::number(value.toInt(), 16); //.toHex();
            break;
        case QJsonTreeItem::FLOAT:
            if (len == 4)
                tmp = QByteArray::number(value.toFloat()).toHex();
            else if (len == 8)
                tmp = QByteArray::number(value.toDouble()).toHex();
            break;
        case QJsonTreeItem::DATE:
            break;
        }
        /*if (tmp.size() > len) {
            QString errMsg = QString("Error: serialization, len = %1, tmp.size() = %2").arg(len).arg(tmp.size());
            qCritical() << errMsg;
            std::cout << qPrintable(errMsg);
            return {};
        }*//*
        while (tmp.size() < len)
            tmp.prepend(1, '0');
        arr.push_back(tmp);
    }

    qDebug() << arr;
    qDebug() << QString::fromLatin1(arr).toUtf8();
    std::cout << QString::fromLatin1(arr).toUtf8().toStdString() << std::endl;

    return arr;
}*/

/*QByteArray QJsonModel::serialize() const
{
    QByteArray arr;

    return arr;
}*/

QJsonValue QJsonModel::genJson(QJsonTreeItem * item) const
{
    auto type   = item->type();
    int  nchild = item->childCount();

    if (QJsonValue::Object == type) {
        QJsonObject jo;
        for (int i = 0; i < nchild; ++i) {
            auto ch = item->child(i);
            auto key = ch->key();
            jo.insert(key, genJson(ch));
        }
        return  jo;
    } else if (QJsonValue::Array == type) {
        QJsonArray arr;
        for (int i = 0; i < nchild; ++i) {
            auto ch = item->child(i);
            arr.append(genJson(ch));
        }
        return arr;
    } else {
        QJsonValue va;
        switch(item->value().type()){
        case QVariant::Bool: {
            va = item->value().toBool();
            break;
        }
        default:
            va = item->value().toString();
            break;
        }
        (item->value());
        return va;
    }
}
