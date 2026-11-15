from pathlib import Path
root=Path(__file__).resolve().parents[1]
src=(root/'src/DashboardRuntime.cpp').read_text()
hdr=(root/'include/DashboardRuntime.h').read_text()
for token in ('choice_selector', 'queueChoiceAction', 'choiceEvent', 'oRequest["choice"]', 'iMaxWidgetsPerPage = 40'):
    assert token in src or token in hdr, token
assert 'json_pool_alloc_failed' in src
assert 'bDirectActionConfigured ||' in src
print('OK selector HTTP + JSON allocation retained')
