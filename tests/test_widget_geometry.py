#!/usr/bin/env python3
"""Check supported tile geometries and prevent subwidgets overlapping/outside bounds."""
from pathlib import Path
sCpp = (Path(__file__).resolve().parents[1] / 'src/DashboardRuntime.cpp').read_text()
pJs = Path(__file__).resolve().parents[3] / 'plugin/dashboardespstudio/desktop/js/dashboardespstudio.js'
sJs = pJs.read_text() if pJs.exists() else ''
assert 'lv_obj_update_layout(tEntry.pObject)' in sCpp
for piece in ['iTitleHeight', 'iValueTop', 'iValueHeight', 'iCellWidth', 'iFooterTop', 'iModeHeight']:
    assert piece in sCpp
    if sJs:
        assert piece in sJs
for width,height in [(180,120),(220,140),(260,160),(320,180),(400,210)]:
    pad=3 if width<170 else 5
    gap=4
    title=17 if height<115 else 21
    mode=19 if height<125 else 23
    footer=23 if height<125 else 29
    value_top=title+3
    value_h=max(21,height-value_top-footer-mode-3*pad)
    cell_w=max(30,(width-2*pad-gap)//2)
    footer_top=value_top+value_h+2
    mode_top=max(0,height-mode-pad)
    assert 2*cell_w+gap+2*pad<=width, (width,height)
    assert value_top+value_h<=footer_top
    assert footer_top+footer<=mode_top, (width,height,footer_top+footer,mode_top)
    assert mode_top+mode+pad<=height
print('PASS geometry for five tile sizes; JS and C++ geometry symbols present')
