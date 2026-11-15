from pathlib import Path
s=Path(__file__).resolve().parents[1]
c=(s/'src/DashboardRuntime.cpp').read_text()
ini=(s/'platformio.ini').read_text()
assert c.count('min<uint32_t>(255U,')==3
assert 'min(255U,' not in c
assert '0.3.31' in ini
print('PASS min type fix V0.3.31')
