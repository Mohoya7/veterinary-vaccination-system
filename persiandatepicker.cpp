#include "persiandatepicker.h"
#include "persiandate.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QPushButton>
#include <QSet>
#include <QMouseEvent>

const QStringList PersianDatePicker::kMonths = {
    "فروردین", "اردیبهشت", "خرداد", "تیر",
    "مرداد", "شهریور", "مهر", "آبان",
    "آذر", "دی", "بهمن", "اسفند"
};
const int PersianDatePicker::kMinY;
const int PersianDatePicker::kMaxY;

static const char* kComboStyle = R"(
    QComboBox {
        border: 1px solid #A5D6A7; border-radius: 6px;
        padding: 6px 8px; font-size: 13px;
        background: #F9FBF9; color: #212121; min-height: 34px;
    }
    QComboBox:hover, QComboBox:focus { border-color: #2E7D32; background: white; }
    QComboBox::drop-down { border: none; width: 20px; }
    QComboBox::down-arrow { image: url(:/icons/chevron-down.svg); width:12px; height:12px; }
    QComboBox QAbstractItemView {
        background: white; border: 1px solid #C8E6C9; border-radius: 6px;
        selection-background-color: #E8F5E9; selection-color: #2E7D32;
        font-size: 13px; outline: none;
    }
    QComboBox QAbstractItemView::item { padding: 6px 10px; min-height: 30px; }
    QComboBox QAbstractItemView QScrollBar:vertical {
        background: transparent; width: 6px; margin: 4px 2px;
        border-radius: 3px;
    }
    QComboBox QAbstractItemView QScrollBar::handle:vertical {
        background: rgba(46,125,50,160); border-radius: 3px; min-height: 24px;
    }
    QComboBox QAbstractItemView QScrollBar::handle:vertical:hover {
        background: rgba(46,125,50,220);
    }
    QComboBox QAbstractItemView QScrollBar::add-line:vertical,
    QComboBox QAbstractItemView QScrollBar::sub-line:vertical { height: 0px; }
    QComboBox QAbstractItemView QScrollBar::add-page:vertical,
    QComboBox QAbstractItemView QScrollBar::sub-page:vertical { background: transparent; }
)";

// ── constructor ───────────────────────────────────────────────────────────────

PersianDatePicker::PersianDatePicker(QWidget* parent)
    : QWidget(parent)
{
    setLayoutDirection(Qt::RightToLeft);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);
    lay->setSpacing(0);
    lay->setAlignment(Qt::AlignVCenter);

    m_field = new QLineEdit(this);
    m_field->setReadOnly(true);
    m_field->setLayoutDirection(Qt::RightToLeft);
    m_field->setCursor(Qt::PointingHandCursor);
    m_field->setPlaceholderText("انتخاب تاریخ");
    m_field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_field->setStyleSheet(R"(
        QLineEdit {
            border: 1px solid #A5D6A7; border-radius: 6px;
            padding: 4px 10px; font-size: 13px;
            background: #F9FBF9; color: #212121;
        }
        QLineEdit:hover { border-color: #2E7D32; background: white; }
    )");
    lay->addWidget(m_field, 0, Qt::AlignVCenter);

    m_field->installEventFilter(this);
    setDate(QDate::currentDate());
}

// ── eventFilter ───────────────────────────────────────────────────────────────

bool PersianDatePicker::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_field &&
        ev->type() == QEvent::MouseButtonPress &&
        !m_readOnly) {
        openDialog();
        return true;
    }
    return QWidget::eventFilter(obj, ev);
}

// ── openDialog ────────────────────────────────────────────────────────────────

void PersianDatePicker::openDialog()
{
    // ── Build Dialog ──────────────────────────────────────────────────────────
    QDialog dlg(this->window());
    dlg.setWindowTitle("انتخاب تاریخ");
    dlg.setLayoutDirection(Qt::RightToLeft);
    dlg.setFixedWidth(340);
    dlg.setStyleSheet(R"(
        QDialog { background: #F1F8E9; }
    )");

    auto* vlay = new QVBoxLayout(&dlg);
    vlay->setContentsMargins(0,0,0,0);
    vlay->setSpacing(0);

    // ── Header ──────────────────────────────────────────────────────────────────
    auto* hdr = new QWidget;
    hdr->setFixedHeight(48);
    hdr->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                stop:0 #2E7D32, stop:1 #1B5E20);
            border-top-left-radius: 0px;
            border-top-right-radius: 0px;
        }
    )");
    auto* hdrLay = new QHBoxLayout(hdr);
    hdrLay->setContentsMargins(16,0,16,0);

    auto* ttl = new QLabel("انتخاب تاریخ");
    ttl->setStyleSheet(
        "color:white;font-size:14px;font-weight:500;"
        "background:transparent;border:none;");
    hdrLay->addWidget(ttl);
    hdrLay->addStretch();
    vlay->addWidget(hdr);

    // ── Body — Three Combos ──────────────────────────────────────────────────────
    auto* body = new QWidget;
    body->setStyleSheet("background:white;");
    auto* bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(20,20,20,16);
    bodyLay->setSpacing(14);

    // Row of three combos
    auto* row = new QWidget;
    row->setStyleSheet("background:transparent;");
    auto* rowLay = new QHBoxLayout(row);
    rowLay->setContentsMargins(0,0,0,0);
    rowLay->setSpacing(8);

    auto makeCol = [&](const QString& lbl, int stretch) -> QComboBox* {
        auto* col = new QWidget;
        col->setStyleSheet("background:transparent;");
        auto* cl = new QVBoxLayout(col);
        cl->setContentsMargins(0,0,0,0);
        cl->setSpacing(4);
        auto* l = new QLabel(lbl);
        l->setStyleSheet(
            "font-size:11px;color:#757575;background:transparent;border:none;");
        l->setAlignment(Qt::AlignLeft);
        auto* cb = new QComboBox;
        cb->setStyleSheet(kComboStyle);
        cb->setLayoutDirection(Qt::RightToLeft);
        cb->setMaxVisibleItems(6);
        cl->addWidget(l);
        cl->addWidget(cb);
        rowLay->addWidget(col, stretch);
        return cb;
    };

    // RTL: Year | Month | Day
    QComboBox* cbYear  = makeCol("سال", 2);
    QComboBox* cbMonth = makeCol("ماه", 2);
    QComboBox* cbDay   = makeCol("روز", 1);

    bodyLay->addWidget(row);

    // ── Fill Combos ──────────────────────────────────────────────────────
    for (int y = kMinY; y <= kMaxY; ++y)
        cbYear->addItem(farsi(y), y);
    for (int i = 0; i < 12; ++i)
        cbMonth->addItem(kMonths[i], i+1);

    // Sync day with selected month and year
    auto syncDay = [&]() {
        int jy  = cbYear->currentData().toInt();
        int jm  = cbMonth->currentData().toInt();
        int maxD = daysInMonth(jy, jm);
        int curD = qMin(cbDay->currentData().toInt(), maxD);
        cbDay->blockSignals(true);
        cbDay->clear();
        for (int d = 1; d <= maxD; ++d)
            cbDay->addItem(farsi(d, 2), d);
        cbDay->setCurrentIndex(qMax(0, curD - 1));
        cbDay->blockSignals(false);
    };

    // Initial value from current state
    cbYear->setCurrentIndex(qBound(0, m_jy - kMinY, cbYear->count()-1));
    cbMonth->setCurrentIndex(m_jm - 1);
    syncDay();
    cbDay->setCurrentIndex(m_jd - 1);

    // Rebuild day when year or month changes
    QObject::connect(cbYear,  QOverload<int>::of(&QComboBox::currentIndexChanged),
                      &dlg, [&](int) { syncDay(); });
    QObject::connect(cbMonth, QOverload<int>::of(&QComboBox::currentIndexChanged),
                      &dlg, [&](int) { syncDay(); });

    // ── Error ──────────────────────────────────────────────────────────────────
    auto* errLbl = new QLabel;
    errLbl->setAlignment(Qt::AlignCenter);
    errLbl->setStyleSheet(
        "font-size:11px;color:#C62828;background:transparent;border:none;");
    errLbl->hide();
    bodyLay->addWidget(errLbl);

    vlay->addWidget(body);

    // ── Footer — Buttons ──────────────────────────────────────────────────────
    auto* footer = new QWidget;
    footer->setStyleSheet(
        "background:white;border-top:1px solid #E8F5E9;");
    footer->setFixedHeight(56);
    auto* footerLay = new QHBoxLayout(footer);
    footerLay->setContentsMargins(16,0,16,0);
    footerLay->setSpacing(8);

    auto* btnOk = new QPushButton("تایید");
    btnOk->setFixedHeight(36);
    btnOk->setMinimumWidth(90);
    btnOk->setCursor(Qt::PointingHandCursor);
    btnOk->setStyleSheet(R"(
        QPushButton {
            background:#2E7D32; color:white; border:none;
            border-radius:6px; font-size:13px; font-weight:500;
        }
        QPushButton:hover { background:#1B5E20; }
    )");

    auto* btnNo = new QPushButton("لغو");
    btnNo->setFixedHeight(36);
    btnNo->setMinimumWidth(90);
    btnNo->setCursor(Qt::PointingHandCursor);
    btnNo->setStyleSheet(R"(
        QPushButton {
            background:white; color:#757575;
            border:1px solid #E0E0E0; border-radius:6px; font-size:13px;
        }
        QPushButton:hover { background:#F5F5F5; }
    )");

    footerLay->addWidget(btnOk);
    footerLay->addWidget(btnNo);
    footerLay->addStretch();
    vlay->addWidget(footer);

    // ── Button Signals ───────────────────────────────────────────────────────
    QObject::connect(btnOk, &QPushButton::clicked, &dlg, [&]() {
        int jy = cbYear->currentData().toInt();
        int jm = cbMonth->currentData().toInt();
        int jd = cbDay->currentData().toInt();

        // Validation
        if (jd > daysInMonth(jy, jm)) {
            errLbl->setText("Invalid day selected");
            errLbl->show();
            return;
        }
        m_jy = jy; m_jm = jm; m_jd = jd;
        dlg.accept();
    });
    QObject::connect(btnNo, &QPushButton::clicked, &dlg, &QDialog::reject);

    // ── Show Dialog ─────────────────────────────────────────────────────────
    if (dlg.exec() == QDialog::Accepted) {
        updateField();
        emit dateChanged(date());
    }
}

// ── public API ────────────────────────────────────────────────────────────────

void PersianDatePicker::setDate(const QDate& g)
{
    if (!g.isValid()) return;
    int jy, jm, jd;
    if (!PersianDate::toJalali(g, jy, jm, jd)) return;
    m_jy = jy; m_jm = jm; m_jd = jd;
    updateField();
}

QDate PersianDatePicker::date() const
{
    return PersianDate::fromJalali(m_jy, m_jm, m_jd);
}

void PersianDatePicker::setReadOnly(bool ro)
{
    m_readOnly = ro;
    m_field->setCursor(ro ? Qt::ArrowCursor : Qt::PointingHandCursor);
    m_field->setStyleSheet(ro
        ? "QLineEdit{border:1px solid #E0E0E0;border-radius:6px;"
          "padding:7px 10px;font-size:13px;background:#F5F5F5;"
          "color:#9E9E9E;min-height:36px;}"
        : "QLineEdit{border:1px solid #A5D6A7;border-radius:6px;"
          "padding:7px 10px;font-size:13px;background:#F9FBF9;"
          "color:#212121;min-height:36px;}"
          "QLineEdit:hover{border-color:#2E7D32;background:white;}");
}

// ── helpers ───────────────────────────────────────────────────────────────────

void PersianDatePicker::updateField()
{
    m_field->setText(farsi(m_jy) + "/" + farsi(m_jm,2) + "/" + farsi(m_jd,2));
}

bool PersianDatePicker::isLeapYear(int jy) const
{
    const auto& tbl = PersianDate::nowruzTable();
    if (!tbl.contains(jy) || !tbl.contains(jy + 1))
        return false;
    return tbl[jy].daysTo(tbl[jy + 1]) == 366;
}

int PersianDatePicker::daysInMonth(int jy, int jm) const
{
    if (jm <= 6)  return 31;
    if (jm <= 11) return 30;
    return isLeapYear(jy) ? 30 : 29;
}

QString PersianDatePicker::farsi(int n, int w) const
{
    QString s = w > 0 ? QString("%1").arg(n, w, 10, QChar('0'))
                      : QString::number(n);
    QString o; o.reserve(s.size());
    for (QChar c : s)
        o += (c>='0'&&c<='9') ? QChar(0x06F0+(c.unicode()-'0')) : c;
    return o;
}