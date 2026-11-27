import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.qgis
import org.qfield.core
import org.qfield.gui

/**
 * \ingroup qml_gui
 *
 * Attribute table of a vector layer, as in QGIS: filter by expression, sort by
 * tapping a column header and edit a value by tapping its cell.
 */
Page {
  id: attributeTable

  property alias layer: tableModel.layer

  signal finished

  visible: false
  focus: visible
  leftPadding: mainWindow.sceneLeftMargin
  rightPadding: mainWindow.sceneRightMargin

  function open(vectorLayer, filter) {
    tableModel.layer = vectorLayer;
    filterField.text = filter || "";
    tableModel.filterExpression = filterField.text;
    visible = true;
  }

  QfAttributeTableModel {
    id: tableModel
  }

  Keys.onReleased: event => {
    if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
      event.accepted = true;
      attributeTable.finished();
    }
  }

  header: QfPageHeader {
    title: tableModel.layer ? qsTr("Attribute table: %1").arg(tableModel.layer.name) : qsTr("Attribute table")

    showBackButton: true
    showApplyButton: false
    showCancelButton: false

    topMargin: mainWindow.sceneTopMargin

    onFinished: attributeTable.finished()
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    anchors.bottomMargin: 8 + mainWindow.sceneBottomMargin
    spacing: 8

    RowLayout {
      Layout.fillWidth: true
      spacing: 4

      QfTextField {
        id: filterField
        Layout.fillWidth: true
        placeholderText: qsTr("Filter expression, e.g. \"name\" LIKE '%river%'")
        inputMethodHints: Qt.ImhNoPredictiveText
        onAccepted: tableModel.filterExpression = text
      }

      QfToolButton {
        iconSource: QfTheme.getThemeVectorIcon("ic_check_white_24dp")
        iconColor: QfTheme.mainOverlayColor
        bgcolor: QfTheme.mainColor
        round: true
        onClicked: tableModel.filterExpression = filterField.text
      }

      QfToolButton {
        visible: tableModel.filterExpression !== ""
        iconSource: QfTheme.getThemeVectorIcon("ic_close_white_24dp")
        iconColor: QfTheme.mainTextColor
        round: true
        onClicked: {
          filterField.text = "";
          tableModel.filterExpression = "";
        }
      }

      QfToolButton {
        // Field calculator
        visible: tableModel.editable && tableView.rows > 0
        iconSource: QfTheme.getThemeVectorIcon("ic_expression_black_24dp")
        iconColor: QfTheme.mainTextColor
        round: true
        onClicked: calculatorDialog.openCalculator()
      }
    }

    Label {
      Layout.fillWidth: true
      visible: tableModel.errorString !== ""
      text: tableModel.errorString
      color: QfTheme.errorColor
      font: QfTheme.tipFont
      wrapMode: Text.Wrap
    }

    Label {
      Layout.fillWidth: true
      font: QfTheme.tipFont
      color: QfTheme.secondaryTextColor
      wrapMode: Text.Wrap
      text: {
        let summary = qsTr("%n feature(s)", "", tableView.rows);
        if (tableModel.limitReached) {
          summary += " — " + qsTr("only the first %1 are shown, use a filter to narrow them down").arg(tableModel.featureLimit);
        }
        if (tableModel.editable) {
          summary += " — " + qsTr("tap a value to edit it");
        }
        return summary;
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: QfTheme.controlBackgroundColor
      border.color: QfTheme.controlBorderColor
      border.width: 1
      clip: true

      HorizontalHeaderView {
        id: headerView
        anchors.left: tableView.left
        anchors.right: parent.right
        anchors.top: parent.top
        syncView: tableView
        clip: true

        delegate: Rectangle {
          implicitWidth: 160
          implicitHeight: 44
          color: QfTheme.groupBoxBackgroundColor
          border.color: QfTheme.controlBorderColor
          border.width: 1

          Text {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            font.pointSize: QfTheme.tipFont.pointSize
            font.bold: true
            color: QfTheme.mainTextColor
            text: {
              const title = tableModel.columnTitle(column);
              if (tableModel.sortColumn !== column)
                return title;
              return title + (tableModel.sortAscending ? " ▲" : " ▼");
            }
          }

          MouseArea {
            anchors.fill: parent
            onClicked: tableModel.sortByColumn(column)
          }
        }
      }

      TableView {
        id: tableView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: headerView.bottom
        anchors.bottom: parent.bottom
        model: tableModel
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        columnWidthProvider: column => 160
        ScrollBar.vertical: QfScrollBar {}
        ScrollBar.horizontal: QfScrollBar {}

        delegate: Rectangle {
          implicitWidth: 160
          implicitHeight: 40
          color: row % 2 === 0 ? QfTheme.controlBackgroundColor : QfTheme.controlBackgroundAlternateColor
          border.color: QfTheme.controlBorderColor
          border.width: 0.5

          Text {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            font: QfTheme.tipFont
            color: model.isNull ? QfTheme.secondaryTextColor : QfTheme.mainTextColor
            font.italic: model.isNull
            text: model.display
          }

          MouseArea {
            anchors.fill: parent
            enabled: model.fieldEditable
            onClicked: editDialog.edit(row, column, model.isNull ? "" : model.value)
          }
        }
      }

      Label {
        anchors.centerIn: parent
        visible: tableView.rows === 0 && tableModel.errorString === ""
        text: tableModel.filterExpression !== "" ? qsTr("No feature matches the filter") : qsTr("The layer has no features")
        color: QfTheme.secondaryTextColor
        font: QfTheme.defaultFont
      }
    }
  }

  QfDialog {
    id: calculatorDialog

    parent: mainWindow.contentItem
    width: Math.min(mainWindow.width - 40, 460)
    title: qsTr("Field calculator")

    property var editableColumns: []

    function openCalculator() {
      const columns = [];
      for (let c = 0; c < tableView.columns; c++) {
        if (tableModel.isColumnEditable(c)) {
          columns.push({ "column": c, "title": tableModel.columnTitle(c) });
        }
      }
      editableColumns = columns;
      if (columns.length === 0) {
        displayToast(qsTr("This layer has no editable field"), "warning");
        return;
      }
      open();
    }

    ColumnLayout {
      width: parent.width
      spacing: 6

      Label {
        text: qsTr("Field to update")
        font: QfTheme.tipFont
        color: QfTheme.secondaryTextColor
      }

      QfComboBox {
        id: calculatorField
        Layout.fillWidth: true
        model: calculatorDialog.editableColumns
        textRole: "title"
      }

      Label {
        text: qsTr("Expression")
        font: QfTheme.tipFont
        color: QfTheme.secondaryTextColor
      }

      QfTextField {
        id: calculatorExpression
        Layout.fillWidth: true
        placeholderText: qsTr("e.g. $area / 10000")
        inputMethodHints: Qt.ImhNoPredictiveText
      }

      Label {
        Layout.fillWidth: true
        text: qsTr("The value is calculated for the %n feature(s) listed in the table.", "", tableView.rows)
        font: QfTheme.tipFont
        color: QfTheme.secondaryTextColor
        wrapMode: Text.Wrap
      }
    }

    onAccepted: {
      const target = editableColumns[calculatorField.currentIndex];
      if (!target || calculatorExpression.text.trim() === "") {
        return;
      }
      const updated = tableModel.calculateColumn(target.column, calculatorExpression.text);
      if (updated >= 0) {
        displayToast(qsTr("%n feature(s) updated", "", updated));
      } else {
        displayToast(tableModel.errorString, "error");
      }
    }
  }

  QfDialog {
    id: editDialog

    property int row: -1
    property int column: -1

    parent: mainWindow.contentItem
    width: Math.min(mainWindow.width - 40, 420)
    title: column >= 0 ? tableModel.columnTitle(column) : ""

    function edit(r, c, value) {
      row = r;
      column = c;
      valueField.text = value === undefined || value === null ? "" : String(value);
      open();
      valueField.forceActiveFocus();
    }

    ColumnLayout {
      width: parent.width
      spacing: 6

      QfTextField {
        id: valueField
        Layout.fillWidth: true
        onAccepted: editDialog.accept()
      }

      Label {
        Layout.fillWidth: true
        text: qsTr("Leave empty to set the value to NULL.")
        font: QfTheme.tipFont
        color: QfTheme.secondaryTextColor
        wrapMode: Text.Wrap
      }
    }

    onAccepted: {
      if (tableModel.setValue(row, column, valueField.text)) {
        displayToast(qsTr("Value saved"));
      } else if (tableModel.errorString !== "") {
        displayToast(tableModel.errorString, "error");
      }
    }
  }
}
