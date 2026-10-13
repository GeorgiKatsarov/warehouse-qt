#include "ReportExport.h"
#include "ProductTableModel.h"
#include <QDataStream>
#include <QSaveFile>
#include <QHash>
#include <QCoreApplication>
#include <algorithm>
#include <cmath>

namespace {
QString text(const char *value) { return QCoreApplication::translate("ReportExport",value); }
bool contains(const std::vector<int> &columns, int column)
{
    return std::find(columns.begin(),columns.end(),column)!=columns.end();
}
QByteArray record(quint16 code, const QByteArray &data)
{
    QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly); stream.setByteOrder(QDataStream::LittleEndian);
    stream << code << quint16(data.size()); bytes.append(data); return bytes;
}
QByteArray words(std::initializer_list<quint16> values)
{
    QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly); stream.setByteOrder(QDataStream::LittleEndian);
    for (quint16 value : values) stream << value;
    return bytes;
}
QByteArray unicode(const QString &value)
{
    QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly); stream.setByteOrder(QDataStream::LittleEndian);
    for (QChar character : value) stream << character.unicode();
    return bytes;
}
QByteArray bof(quint16 type)
{
    QByteArray bytes = words({0x0600,type,0x0dbb,0x07cc});
    QDataStream stream(&bytes,QIODevice::Append); stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32(0x41) << quint32(0x06); return record(0x0809,bytes);
}
QByteArray directoryEntry(QString name, bool root, quint32 start, quint64 size)
{
    QByteArray bytes(128,'\0'); const QByteArray nameBytes = unicode(name+QChar(0));
    std::copy(nameBytes.begin(),nameBytes.end(),bytes.begin());
    QByteArray tail; QDataStream stream(&tail,QIODevice::WriteOnly); stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint16(nameBytes.size()) << quint8(root ? 5 : 2) << quint8(1)
        << quint32(0xffffffff) << quint32(0xffffffff) << quint32(root ? 1 : 0xffffffff);
    tail.append(QByteArray(16+4+8+8,'\0'));
    QDataStream end(&tail,QIODevice::Append); end.setByteOrder(QDataStream::LittleEndian);
    end << start << size;
    std::copy(tail.begin(),tail.end(),bytes.begin()+64); return bytes;
}
// One Workbook stream in a version-3 Compound File, using ordinary 512-byte sectors.
QByteArray compound(QByteArray workbook)
{
    const quint64 streamSize = std::max<qsizetype>(4096,workbook.size());
    const quint32 sectors = quint32((streamSize+511)/512);
    workbook.append(QByteArray(int(sectors*512)-workbook.size(),'\0'));
    int fatCount = 1;
    while (fatCount*128 < int(sectors)+1+fatCount) ++fatCount;
    if (fatCount>109) return {};
    QByteArray header = QByteArray::fromHex("d0cf11e0a1b11ae1"); header.append(QByteArray(16,'\0'));
    QDataStream stream(&header,QIODevice::Append); stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint16(0x3e) << quint16(3) << quint16(0xfffe) << quint16(9) << quint16(6);
    header.append(QByteArray(6,'\0'));
    QDataStream rest(&header,QIODevice::Append); rest.setByteOrder(QDataStream::LittleEndian);
    rest << quint32(0) << quint32(fatCount) << sectors << quint32(0) << quint32(4096)
         << quint32(0xfffffffe) << quint32(0) << quint32(0xfffffffe) << quint32(0);
    for (int i=0;i<109;++i) rest << (i<fatCount ? sectors+1+quint32(i) : quint32(0xffffffff));
    QByteArray directory = directoryEntry("Root Entry",true,0xfffffffe,0)
        + directoryEntry("Workbook",false,0,streamSize);
    directory.append(QByteArray(256,'\0'));
    QByteArray fat; QDataStream allocation(&fat,QIODevice::WriteOnly); allocation.setByteOrder(QDataStream::LittleEndian);
    for (int i=0;i<fatCount*128;++i) {
        quint32 next = 0xffffffff;
        if (quint32(i)<sectors) next = quint32(i)+1==sectors ? 0xfffffffe : quint32(i)+1;
        else if (quint32(i)==sectors) next = 0xfffffffe;
        else if (quint32(i)<=sectors+quint32(fatCount)) next = 0xfffffffd;
        allocation << next;
    }
    return header+workbook+directory+fat;
}
}

QString reportHtml(const Report &report)
{
    QString html = "<html><head><meta charset='utf-8'><style>body{font-family:Arial;color:#17243a;font-size:10pt}"
        "h1{font-size:20pt;color:#1d4ed8}p{color:#475569}table{border-collapse:collapse;width:100%}"
        "th{background:#e8efff;text-align:left}th,td{border:1px solid #cbd5e1;padding:7px}</style></head><body>";
    html += "<h1>"+report.title().toHtmlEscaped()+"</h1><p>"+reportTypeName(report.type()).toHtmlEscaped()+"<br>"
        +text("Отчет №")+QString::number(report.id())+" · "+text("Създаден: ")+report.createdAt().toString("dd.MM.yyyy HH:mm")
        +"<br>"+text("Период: ")+report.dateFrom().toString("dd.MM.yyyy")+" — "+report.dateTo().toString("dd.MM.yyyy")+"</p>";
    if (report.type()==ReportType::InventoryValuation || report.type()==ReportType::LowStock)
        html += "<p>"+text("Наличности към момента на създаване на отчета.")+"</p>";
    html += "<table><thead><tr>";
    for (const QString &header : report.headers()) html += "<th>"+header.toHtmlEscaped()+"</th>";
    html += "</tr></thead><tbody>";
    for (const QStringList &row : report.rows()) {
        html += "<tr>";
        for (int column=0;column<row.size();++column)
            html += "<td>"+(contains(report.moneyColumns(),column) ? formatCents(row.at(column).toLongLong()) : row.at(column)).toHtmlEscaped()+"</td>";
        html += "</tr>";
    }
    if (report.rows().empty()) html += "<tr><td colspan='"+QString::number(report.headers().size())+"'>"+text("Няма данни за този отчет.")+"</td></tr>";
    return html+"</tbody></table><p>"+text("Запазен отчет · EUR · Складова система")+"</p></body></html>";
}

QString saveReportXls(const Report &report, const QString &path)
{
    if (report.headers().isEmpty() || report.headers().size()>256 || report.rows().size()>65530)
        return text("XLS поддържа до 65 530 реда данни и 256 колони.");
    for (const QStringList &row : report.rows())
        if (row.size()!=report.headers().size()) return text("Невалидна структура на отчета.");
    std::vector<QStringList> rows = {
        {report.title()},
        {text("Тип"),reportTypeName(report.type())},
        {text("Период"),report.dateFrom().toString(Qt::ISODate)+" — "+report.dateTo().toString(Qt::ISODate)},
        {text("Създаден"),report.createdAt().toString("dd.MM.yyyy HH:mm")},
        report.headers()
    };
    const int firstData = static_cast<int>(rows.size());
    for (const auto &row : report.rows()) rows.push_back(row);
    QHash<QString,int> stringIds; QStringList strings;
    QByteArray cells;
    int labelCount=0;
    for (int row=0;row<static_cast<int>(rows.size());++row) {
        for (int column=0;column<rows.at(row).size();++column) {
            const QString value = rows.at(row).at(column);
            const bool money = row>=firstData && contains(report.moneyColumns(),column);
            const bool numeric = row>=firstData && (money || contains(report.numberColumns(),column));
            bool valid = false; const qint64 integer = value.toLongLong(&valid);
            // Excel has 15 significant digits; preserve larger integers as text instead of losing cents.
            if (numeric && valid && std::abs(double(integer))<1e15) {
                QByteArray data = words({quint16(row),quint16(column),quint16(money ? 16 : 15)});
                QDataStream stream(&data,QIODevice::Append); stream.setByteOrder(QDataStream::LittleEndian);
                stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
                stream << (money ? double(integer)/100.0 : double(integer)); cells += record(0x0203,data);
            } else {
                const QString label = money && valid ? formatCents(integer) : value;
                if (label.size()>4000) return text("Една XLS клетка може да съдържа до 4000 знака в този износ.");
                if (!stringIds.contains(label)) { stringIds.insert(label,strings.size()); strings.append(label); }
                QByteArray data = words({quint16(row),quint16(column),quint16(row==0 || row==4 ? 17 : 15)});
                QDataStream stream(&data,QIODevice::Append); stream.setByteOrder(QDataStream::LittleEndian);
                stream << quint32(stringIds.value(label)); cells += record(0x00fd,data);
                ++labelCount;
            }
        }
    }
    QByteArray globals = bof(5)+record(0x0042,words({1200}))+record(0x0022,words({0}));
    for (int bold : {0,1}) {
        QByteArray font = words({220,0,0x7fff,quint16(bold ? 700 : 400),0});
        font.append(char(0)); font.append(char(0)); font.append(char(0)); font.append(char(0));
        font.append(char(5)); font.append(char(1)); font.append(unicode("Arial")); globals += record(0x0031,font);
    }
    QByteArray format = words({164,8}); format.append(char(1)); format.append(unicode(QString::fromUtf8("0.00 \"€\"")));
    globals += record(0x041e,format);
    for (int index=0;index<18;++index) {
        QByteArray xf = words({quint16(index==17 ? 1 : 0),quint16(index==16 ? 164 : 0),quint16(index==0 ? 0xfff5 : 1)});
        xf.append(QByteArray(14,'\0'));
        if (index==16) xf[9]=char(0x04);
        globals += record(0x00e0,xf);
    }
    globals += record(0x0293,words({0x8000,0}));
    QByteArray sst; QDataStream totals(&sst,QIODevice::WriteOnly); totals.setByteOrder(QDataStream::LittleEndian);
    totals << quint32(labelCount) << quint32(strings.size());
    bool first = true;
    for (const QString &value : strings) {
        QByteArray encoded = words({quint16(value.size())}); encoded.append(char(1)); encoded += unicode(value);
        if (sst.size()+encoded.size()>8224) { globals += record(first ? 0x00fc : 0x003c,sst); first = false; sst.clear(); }
        sst += encoded;
    }
    globals += record(first ? 0x00fc : 0x003c,sst);
    QByteArray bound(4,'\0'); bound.append(char(0)); bound.append(char(0)); bound.append(char(5)); bound.append(char(1));
    bound += unicode(QString::fromUtf8("Отчет"));
    const quint32 sheetOffset = globals.size()+4+bound.size()+4;
    QByteArray offset; QDataStream offsetStream(&offset,QIODevice::WriteOnly); offsetStream.setByteOrder(QDataStream::LittleEndian); offsetStream << sheetOffset;
    std::copy(offset.begin(),offset.end(),bound.begin());
    globals += record(0x0085,bound)+record(0x000a,{});
    QByteArray dimensions; QDataStream dims(&dimensions,QIODevice::WriteOnly); dims.setByteOrder(QDataStream::LittleEndian);
    dims << quint32(0) << quint32(rows.size()) << quint16(0) << quint16(std::max<qsizetype>(2,report.headers().size())) << quint16(0);
    QByteArray sheet = bof(0x10)+record(0x0200,dimensions);
    for (int column=0;column<report.headers().size();++column)
        sheet += record(0x007d,words({quint16(column),quint16(column),quint16(column==1 ? 9000 : 6000),15,0,0}));
    sheet += cells+record(0x023e,words({0x06b6,0,0,0,0,0,0,0,0}))+record(0x000a,{});
    const QByteArray workbook = compound(globals+sheet);
    if (workbook.isEmpty()) return text("Отчетът е твърде голям за XLS износ.");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return text("Файлът не може да се отвори: ")+file.errorString();
    if (file.write(workbook)!=workbook.size() || !file.commit()) return text("Неуспешен запис на XLS: ")+file.errorString();
    return {};
}
