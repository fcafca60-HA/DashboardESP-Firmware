#!/usr/bin/env python3
"""Offline schema and examples sanity checks (not an ESP32 build)."""
from pathlib import Path
import json
import re

sRoot = Path(__file__).resolve().parents[1]
aProject = json.loads((sRoot / 'data/demo_project.json').read_text(encoding='utf-8'))
aValues = json.loads((sRoot / 'data/demo_values.json').read_text(encoding='utf-8'))
sHeader = (sRoot / 'include/DashboardRuntime.h').read_text(encoding='utf-8')
sRuntime = (sRoot / 'src/DashboardRuntime.cpp').read_text(encoding='utf-8')
assert aProject['schema'] == 'dashboardesp/1'
assert aProject['resolution'] == {'width': 480, 'height': 320}
assert len(aProject['pages']) <= 12
assert len(json.dumps(aProject, ensure_ascii=False).encode('utf-8')) <= 24576
assert len(json.dumps(aValues).encode('utf-8')) <= 8192
sPageIds = {aPage['id'] for aPage in aProject['pages']}
aSupported = {'label', 'value', 'button', 'rectangle', 'page_link', 'datetime', 'weather',
              'energy_summary', 'energy_meter', 'symbol'}
aSeen = set()
for aPage in aProject['pages']:
    assert len(aPage['widgets']) <= 24
    for aWidget in aPage['widgets']:
        assert aWidget['type'] in aSupported
        aSeen.add(aWidget['type'])
        assert 0 <= aWidget['x'] < 480 and 0 <= aWidget['y'] < 320
        assert 0 < aWidget['w'] <= 480 - aWidget['x']
        assert 0 < aWidget['h'] <= 320 - aWidget['y']
        if aWidget['type'] == 'page_link':
            assert aWidget['target_page_id'] in sPageIds
assert aSeen == aSupported, (aSupported - aSeen)
for sType in aSupported:
    assert f'"{sType}"' in sRuntime
assert 'X-Dashboard-Token' in sRuntime
assert 'iMaxConfigBytes = 24576' in sHeader
assert '"values"' in json.dumps(aValues)
print('PASS: schema, widgets, geometry, bindings, examples, API token (static only)')
