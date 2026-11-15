from pathlib import Path
s=Path(__file__).resolve().parents[1]
h=(s/'include/DashboardRuntime.h').read_text()
c=(s/'src/DashboardRuntime.cpp').read_text()
ini=(s/'platformio.ini').read_text()
assert 'iMaxWidgetsPerPage = 40' in h
assert 'iJsonCapacity = 24576' in h
assert 'void DashboardRuntime::updateSaver()' in c
assert 'LV_EVENT_PRESSED, this' in c
assert 'saverTouchEvent' in h
assert '0.3.31' in ini
print('PASS firmware saver / widget limit / version')
