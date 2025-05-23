#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <math.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

// 전역 변수
HWND hSlider, hStaticValue, hRadio1, hRadio2, hRadio3, hCheckMute, hCalcButton, hOutput;

double g_selectedInterval = 0.5;
double g_referenceDbValue = 0.0;

#define SLIDER_MIN_POS 0
#define SLIDER_MAX_POS 256
#define DB_MIN_VAL -128.0
#define DB_MAX_VAL 128.0

void UpdateReferenceValueText(HWND hwndDlg) {
    TCHAR buf[64];
    int sliderPos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
    g_referenceDbValue = DB_MIN_VAL + (double)sliderPos;
    _stprintf(buf, _T("기준값: %+.1f dB"), g_referenceDbValue);
    SetWindowText(hStaticValue, buf);
}

void GenerateOutput(HWND hwndDlg) {
    TCHAR output[65536] = _T("");
    TCHAR line[64];
    BOOL muteChecked = (IsDlgButtonChecked(hwndDlg, IDC_CHECK_MUTE) == BST_CHECKED);

    if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1)) {
        g_selectedInterval = 0.2;
    } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2)) {
        g_selectedInterval = 0.5;
    } else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3)) {
        g_selectedInterval = 1.0;
    }
    double currentCalculatedDb;

    for (int i = 0; i < 256; ++i) {
        currentCalculatedDb = g_referenceDbValue - (i * g_selectedInterval);
        if (currentCalculatedDb < DB_MIN_VAL) currentCalculatedDb = DB_MIN_VAL;
        else if (currentCalculatedDb > DB_MAX_VAL) currentCalculatedDb = DB_MAX_VAL;

        TCHAR binaryString[9];
        for (int bit = 7; bit >= 0; --bit) {
            binaryString[7 - bit] = ((i >> bit) & 1) ? _T('1') : _T('0');
        }
        binaryString[8] = _T('\0');

        if (i == 255 && muteChecked) {
            _stprintf(line, _T("%s: Mute\r\n"), binaryString);
        } else {
            _stprintf(line, _T("%s: %+.1f dB\r\n"), binaryString, currentCalculatedDb);
        }
        _tcscat(output, line);
    }

    SetWindowText(hOutput, output);
}

INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_BAR_CLASSES;
            InitCommonControlsEx(&icex);

            hSlider = GetDlgItem(hwndDlg, IDC_SLIDER);
            hStaticValue = GetDlgItem(hwndDlg, IDC_STATIC_VALUE);
            hRadio1 = GetDlgItem(hwndDlg, IDC_RADIO1);
            hRadio2 = GetDlgItem(hwndDlg, IDC_RADIO2);
            hRadio3 = GetDlgItem(hwndDlg, IDC_RADIO3);
            hCheckMute = GetDlgItem(hwndDlg, IDC_CHECK_MUTE);
            hCalcButton = GetDlgItem(hwndDlg, IDC_CALC_BTN);
            hOutput = GetDlgItem(hwndDlg, IDC_OUTPUT_BOX);

            SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(SLIDER_MIN_POS, SLIDER_MAX_POS));
            SendMessage(hSlider, TBM_SETPOS, TRUE, SLIDER_MAX_POS / 2);
            g_referenceDbValue = DB_MIN_VAL + (SLIDER_MAX_POS / 2);

            CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO2);
            UpdateReferenceValueText(hwndDlg);
            GenerateOutput(hwndDlg);
            return TRUE;
        }

        case WM_HSCROLL:
            if ((HWND)lParam == hSlider) {
                UpdateReferenceValueText(hwndDlg);
                // 필요 시 즉시 결과 갱신
                // GenerateOutput(hwndDlg);
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_RADIO1:
                case IDC_RADIO2:
                case IDC_RADIO3:
                    CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO3, LOWORD(wParam));
                    break;
                case IDC_CALC_BTN:
                    GenerateOutput(hwndDlg);
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
    return DialogBox(hInst, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, DlgProc);
}
