#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <math.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

HWND hSlider, hStaticValue, hRadio1, hRadio2, hRadio3, hCheckMute, hCalcButton, hOutput;
HWND hInputDb, hConvertBtn, hOutputBin;
HWND hRadioDb, hRadioBck, hStaticUnitLabel;

double g_selectedInterval = 0.5;
double g_referenceDbValue = 0.0;

#define SLIDER_MIN_POS 0
#define SLIDER_MAX_POS 256
#define DB_MIN_VAL -128.0
#define DB_MAX_VAL 128.0

// 단위별 표시 텍스트
void UpdateUnitLabel(HWND hwndDlg) {
    if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_DB) == BST_CHECKED) {
        SetWindowText(hStaticUnitLabel, _T("단위: dB"));
    }
    else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_BCK) == BST_CHECKED) {
        SetWindowText(hStaticUnitLabel, _T("단위: BCK"));
    }
}

void UpdateReferenceValueText(HWND hwndDlg) {
    TCHAR buf[64];
    int sliderPos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
    g_referenceDbValue = DB_MIN_VAL + (double)sliderPos;
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

    // 단위 확인
    BOOL isDbUnit = (IsDlgButtonChecked(hwndDlg, IDC_RADIO_DB) == BST_CHECKED);

    double currentCalculatedDb;
    for (int i = 0; i < 256; ++i) {
        currentCalculatedDb = g_referenceDbValue - (i * g_selectedInterval);
        if (currentCalculatedDb < DB_MIN_VAL) currentCalculatedDb = DB_MIN_VAL;
        else if (currentCalculatedDb > DB_MAX_VAL) currentCalculatedDb = DB_MAX_VAL;

        TCHAR binaryString[9];
        for (int bit = 7; bit >= 0; --bit)
            binaryString[7 - bit] = ((i >> bit) & 1) ? _T('1') : _T('0');
        binaryString[8] = _T('\0');

        if (i == 255 && muteChecked)
            _stprintf(line, _T("%s: Mute\r\n"), binaryString);
        else {
            if (isDbUnit)
                _stprintf(line, _T("%s: %+.1f dB\r\n"), binaryString, currentCalculatedDb);
            else
                _stprintf(line, _T("%s: %+.1f BCK\r\n"), binaryString, currentCalculatedDb);
        }

        _tcscat(output, line);
    }

    SetWindowText(hOutput, output);
    free(output);
}

void ConvertDbToBinary(HWND hwndDlg) {
    TCHAR inputBuf[32];
    GetWindowText(hInputDb, inputBuf, 32);
    double inputDb = _tstof(inputBuf);

    int index = (int)((g_referenceDbValue - inputDb) / g_selectedInterval + 0.5);
    if (index < 0) index = 0;
    if (index > 255) index = 255;

    TCHAR binaryString[9];
    for (int bit = 7; bit >= 0; --bit)
        binaryString[7 - bit] = ((index >> bit) & 1) ? _T('1') : _T('0');
    binaryString[8] = _T('\0');

    SetWindowText(hOutputBin, binaryString);
}

void CalculateComplementBits(HWND hwndDlg)
{
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

    // 단위 선택
    TCHAR unitLabel[8];
    if (IsDlgButtonChecked(hwndDlg, IDC_COMP_UNIT_PERCENT) == BST_CHECKED) {
        _tcscpy(unitLabel, _T("%"));
    }
    else if (IsDlgButtonChecked(hwndDlg, IDC_COMP_UNIT_VOLT) == BST_CHECKED) {
        _tcscpy(unitLabel, _T("V"));
    }
    else {
        _tcscpy(unitLabel, _T("°C"));
    }

    int maxIndex = (1 << bits) - 1;
    double step = (double)(maxVal - minVal) / maxIndex;

    TCHAR output[65536] = _T("");
    TCHAR line[128];

    for (int i = 0; i <= maxIndex; ++i) {
        double val = minVal + (i * step);

        TCHAR binary[33];
        for (int b = bits - 1; b >= 0; --b)
            binary[bits - 1 - b] = ((i >> b) & 1) ? _T('1') : _T('0');
        binary[bits] = _T('\0');

        _stprintf(line, _T("%2d (%s) → %6.2f %s\r\n"), i, binary, val, unitLabel);
        _tcscat(output, line);
    }

    SetDlgItemText(hwndDlg, IDC_COMP_OUTPUT_BOX, output);
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

        SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(SLIDER_MIN_POS, SLIDER_MAX_POS));
        SendMessage(hSlider, TBM_SETPOS, TRUE, SLIDER_MAX_POS / 2);
        g_referenceDbValue = DB_MIN_VAL + (SLIDER_MAX_POS / 2);

        CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO2);
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
