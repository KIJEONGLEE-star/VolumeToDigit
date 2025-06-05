#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <math.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

#define SLIDER_MIN_POS 0
#define SLIDER_MAX_POS 256
#define DB_MIN_VAL -128.0
#define DB_MAX_VAL 128.0
#define MAX_STR_LEN 256

HWND hSlider, hStaticValue, hRadio1, hRadio2, hRadio3, hCheckMute, hCalcButton, hOutput;
HWND hInputDb, hConvertBtn, hOutputBin;
HWND hRadioDb, hRadioBck, hStaticUnitLabel;
HWND hRadioDirection[2];
HWND hLabelMinus, hLabelPlus, hLabelCenter; // 기존 + 가운데 라벨
HWND hOutputBox;

double g_selectedInterval = 0.5;
double g_referenceDbValue = 0.0;
double step = 0.5;
double minDb = -128.0;
double maxDb = 24.0;
double baseDb = 0.0;
BOOL mute = FALSE;
BOOL isBelow = TRUE; // 기준값 이하(true), 이상(false)

void UpdateUnitLabel(HWND hwndDlg) {
    if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_DB) == BST_CHECKED)
        SetWindowText(hStaticUnitLabel, _T("단위: dB"));
    else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_BCK) == BST_CHECKED)
        SetWindowText(hStaticUnitLabel, _T("단위: BCK"));
}

void UpdateReferenceValueText(HWND hwndDlg) {
    TCHAR buf[64];
    int sliderPos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
    g_referenceDbValue = DB_MIN_VAL + ((DB_MAX_VAL - DB_MIN_VAL) * sliderPos) / (SLIDER_MAX_POS - SLIDER_MIN_POS);
    _stprintf(buf, _T("기준값: %+.1f dB"), g_referenceDbValue);
    SetWindowText(hStaticValue, buf);
}

void GenerateOutput(HWND hwndDlg) {
    TCHAR* output = (TCHAR*)malloc(65536 * sizeof(TCHAR));
    if (!output) return;
    output[0] = _T('\0');

    TCHAR line[64];
    BOOL muteChecked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_MUTE) == BST_CHECKED);

    if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1)) g_selectedInterval = 0.2;
    else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2)) g_selectedInterval = 0.5;
    else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3)) g_selectedInterval = 1.0;

    BOOL isDbUnit = (IsDlgButtonChecked(hwndDlg, IDC_RADIO_DB) == BST_CHECKED);

    for (int i = 0; i < 256; ++i) {
        double val = isBelow ? g_referenceDbValue - (i * g_selectedInterval)
            : g_referenceDbValue + (i * g_selectedInterval);
        if (val < DB_MIN_VAL) val = DB_MIN_VAL;
        if (val > DB_MAX_VAL) val = DB_MAX_VAL;

        TCHAR binary[9];
        for (int b = 7; b >= 0; --b)
            binary[7 - b] = ((i >> b) & 1) ? _T('1') : _T('0');
        binary[8] = _T('\0');

        if (i == 255 && muteChecked)
            _stprintf(line, _T("%s: Mute\r\n"), binary);
        else
            _stprintf(line, _T("%s: %+.1f %s\r\n"), binary, val, isDbUnit ? _T("dB") : _T("BCK"));

        _tcscat(output, line);
    }

    SetWindowText(hOutput, output);
    free(output);
}

void ConvertDbToBinary(HWND hwndDlg) {
    TCHAR inputBuf[32];
    GetWindowText(hInputDb, inputBuf, 32);
    double inputDb = _tstof(inputBuf);

    int index = (int)((isBelow ? (g_referenceDbValue - inputDb) : (inputDb - g_referenceDbValue)) / g_selectedInterval + 0.5);
    if (index < 0) index = 0;
    if (index > 255) index = 255;

    TCHAR binary[9];
    for (int b = 7; b >= 0; --b)
        binary[7 - b] = ((index >> b) & 1) ? _T('1') : _T('0');
    binary[8] = _T('\0');

    SetWindowText(hOutputBin, binary);
}

void CalculateComplementBits(HWND hwndDlg) {
    TCHAR buf[16];
    GetDlgItemText(hwndDlg, IDC_COMP_MIN, buf, 16);
    int minVal = _ttoi(buf);
    GetDlgItemText(hwndDlg, IDC_COMP_MAX, buf, 16);
    int maxVal = _ttoi(buf);
    GetDlgItemText(hwndDlg, IDC_COMP_BITS, buf, 16);
    int bits = _ttoi(buf);

    if (bits <= 0 || bits > 32 || minVal > maxVal) {
        MessageBox(hwndDlg, _T("입력 값이 잘못되었습니다."), _T("오류"), MB_OK | MB_ICONERROR);
        return;
    }

    TCHAR unit[8];
    if (IsDlgButtonChecked(hwndDlg, IDC_COMP_UNIT_PERCENT) == BST_CHECKED)
        _tcscpy(unit, _T("%"));
    else if (IsDlgButtonChecked(hwndDlg, IDC_COMP_UNIT_VOLT) == BST_CHECKED)
        _tcscpy(unit, _T("V"));
    else
        _tcscpy(unit, _T("°C"));

    int maxIdx = (1 << bits) - 1;
    double delta = (double)(maxVal - minVal) / maxIdx;

    TCHAR output[65536] = _T("");
    TCHAR line[128];

    for (int i = 0; i <= maxIdx; ++i) {
        double val = minVal + i * delta;
        TCHAR bin[33];
        for (int b = bits - 1; b >= 0; --b)
            bin[bits - 1 - b] = ((i >> b) & 1) ? _T('1') : _T('0');
        bin[bits] = _T('\0');

        _stprintf(line, _T("%2d (%s) → %6.2f %s\r\n"), i, bin, val, unit);
        _tcscat(output, line);
    }
    SetDlgItemText(hwndDlg, IDC_COMP_OUTPUT_BOX, output);
}

void UpdateStaticValuePosition(HWND hwndDlg)
{
    RECT thumbRect;
    TCHAR buf[64];

    int pos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
    double val = DB_MIN_VAL + ((DB_MAX_VAL - DB_MIN_VAL) * pos) / (SLIDER_MAX_POS - SLIDER_MIN_POS);

    _stprintf(buf, _T("기준값: %+.1f dB"), val);
    SetWindowText(hStaticValue, buf);

    SendMessage(hSlider, TBM_GETTHUMBRECT, 0, (LPARAM)&thumbRect);

    POINT pt = { thumbRect.left, thumbRect.bottom };
    ClientToScreen(hSlider, &pt);
    ScreenToClient(hwndDlg, &pt);

    RECT rcStatic;
    GetWindowRect(hStaticValue, &rcStatic);
    int width = rcStatic.right - rcStatic.left;
    int height = rcStatic.bottom - rcStatic.top;

    int newX = pt.x - width / 2;
    int newY = pt.y + 4;

    RECT rcDlg;
    GetClientRect(hwndDlg, &rcDlg);
    if (newX < 0) newX = 0;
    if (newX + width > rcDlg.right) newX = rcDlg.right - width;

    SetWindowPos(hStaticValue, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void OnCalc(HWND hwnd) {
    mute = (IsDlgButtonChecked(hwnd, IDC_CHECK_MUTE) == BST_CHECKED);
    if (IsDlgButtonChecked(hwnd, IDC_RADIO1)) step = 0.2;
    else if (IsDlgButtonChecked(hwnd, IDC_RADIO2)) step = 0.5;
    else step = 1.0;

    isBelow = (IsDlgButtonChecked(hwnd, IDC_RADIO_DIRECTION_BELOW) == BST_CHECKED);
}

INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_BAR_CLASSES };
        InitCommonControlsEx(&icex);

        hSlider = GetDlgItem(hwndDlg, IDC_SLIDER);
        hStaticValue = GetDlgItem(hwndDlg, IDC_STATIC_VALUE);
        hRadio1 = GetDlgItem(hwndDlg, IDC_RADIO1);
        hRadio2 = GetDlgItem(hwndDlg, IDC_RADIO2);
        hRadio3 = GetDlgItem(hwndDlg, IDC_RADIO3);
        hCheckMute = GetDlgItem(hwndDlg, IDC_CHECK_MUTE);
        hCalcButton = GetDlgItem(hwndDlg, IDC_CALC_BTN);
        hOutput = GetDlgItem(hwndDlg, IDC_OUTPUT_BOX);
        hInputDb = GetDlgItem(hwndDlg, IDC_INPUT_DB);
        hConvertBtn = GetDlgItem(hwndDlg, IDC_CONVERT_BTN);
        hOutputBin = GetDlgItem(hwndDlg, IDC_OUTPUT_BIN);

        hRadioDb = GetDlgItem(hwndDlg, IDC_RADIO_DB);
        hRadioBck = GetDlgItem(hwndDlg, IDC_RADIO_BCK);
        hStaticUnitLabel = GetDlgItem(hwndDlg, IDC_COMP_UNIT_LABEL);

        hRadioDirection[0] = GetDlgItem(hwndDlg, IDC_RADIO_DIRECTION_BELOW);
        hRadioDirection[1] = GetDlgItem(hwndDlg, IDC_RADIO_DIRECTION_ABOVE);



        SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(SLIDER_MIN_POS, SLIDER_MAX_POS));
        SendMessage(hSlider, TBM_SETPOS, TRUE, SLIDER_MAX_POS / 2);
        g_referenceDbValue = DB_MIN_VAL + (SLIDER_MAX_POS / 2);

        CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO2);
        CheckRadioButton(hwndDlg, IDC_RADIO_DIRECTION_BELOW, IDC_RADIO_DIRECTION_ABOVE, IDC_RADIO_DIRECTION_BELOW);
        CheckRadioButton(hwndDlg, IDC_RADIO_DB, IDC_RADIO_BCK, IDC_RADIO_DB);
        CheckRadioButton(hwndDlg, IDC_COMP_UNIT_PERCENT, IDC_COMP_UNIT_VOLT, IDC_COMP_UNIT_CELSIUS);
        UpdateReferenceValueText(hwndDlg);
        UpdateUnitLabel(hwndDlg);
        GenerateOutput(hwndDlg);
        return TRUE;
    }
    case WM_HSCROLL:
        if ((HWND)lParam == hSlider) {
            UpdateReferenceValueText(hwndDlg);
            // OnCalc(hwndDlg);
        }
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_RADIO1:
        case IDC_RADIO2:
        case IDC_RADIO3:
            CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO3, LOWORD(wParam));
            break;
        case IDC_RADIO_DB:
        case IDC_RADIO_BCK:
            CheckRadioButton(hwndDlg, IDC_RADIO_DB, IDC_RADIO_BCK, LOWORD(wParam));
            UpdateUnitLabel(hwndDlg);
            break;
        case IDC_CALC_BTN:
            GenerateOutput(hwndDlg);
            break;
        case IDC_CONVERT_BTN:
            ConvertDbToBinary(hwndDlg);
            break;
        case IDC_COMP_CALC_BTN:
            CalculateComplementBits(hwndDlg);
            break;
        case IDC_RADIO_DIRECTION_BELOW:
        case IDC_RADIO_DIRECTION_ABOVE:
            CheckRadioButton(hwndDlg, IDC_RADIO_DIRECTION_BELOW, IDC_RADIO_DIRECTION_ABOVE, LOWORD(wParam));
            OnCalc(hwndDlg);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(hwndDlg, 0);
        return TRUE;
    }
    return FALSE;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    return (int)DialogBox(hInst, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, DlgProc);
}
