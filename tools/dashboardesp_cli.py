#!/usr/bin/env python3
"""DashboardESP local API client. No external Python dependencies.

Usage:
  python dashboardesp_cli.py --host 192.168.1.50 --token TOKEN status
  python dashboardesp_cli.py --host 192.168.1.50 --token TOKEN publish data/demo_project.json
  python dashboardesp_cli.py --host 192.168.1.50 --token TOKEN values data/demo_values.json
  python dashboardesp_cli.py --host 192.168.1.50 --token TOKEN actions
  python dashboardesp_cli.py --host 192.168.1.50 --token TOKEN page controls
"""

import argparse
import json
import urllib.error
import urllib.request
from pathlib import Path


def SendRequest(sHost, iPort, sToken, sPath, sMethod, pData=None):
    sUrl = f"http://{sHost}:{iPort}{sPath}"
    aHeaders = {"X-Dashboard-Token": sToken, "Content-Type": "application/json"}
    pRequest = urllib.request.Request(sUrl, data=pData, headers=aHeaders, method=sMethod)

    try:
        with urllib.request.urlopen(pRequest, timeout=15) as pResponse:
            return pResponse.read().decode("utf-8")
    except urllib.error.HTTPError as pError:
        raise SystemExit(f"HTTP {pError.code}: {pError.read().decode('utf-8', errors='replace')}") from pError


def Main():
    pParser = argparse.ArgumentParser(description="DashboardESP HTTP client")
    pParser.add_argument("--host", required=True)
    pParser.add_argument("--port", type=int, default=8090)
    pParser.add_argument("--token", required=True)
    pSub = pParser.add_subparsers(dest="sAction", required=True)
    pSub.add_parser("status")
    pSub.add_parser("actions")
    pSub.add_parser("reset-wifi")
    pPublish = pSub.add_parser("publish")
    pPublish.add_argument("sFile")
    pValues = pSub.add_parser("values")
    pValues.add_argument("sFile")
    pPage = pSub.add_parser("page")
    pPage.add_argument("sPageId")
    aArgs = pParser.parse_args()
    sPath = {"status": "/api/status", "publish": "/api/config", "values": "/api/values",
             "actions": "/api/actions", "page": "/api/page", "reset-wifi": "/api/reset-wifi"}[aArgs.sAction]
    sMethod = "GET" if aArgs.sAction in ("status", "actions") else "POST"
    pData = None

    if aArgs.sAction in ("publish", "values"):
        pData = Path(aArgs.sFile).read_bytes()
        json.loads(pData.decode("utf-8"))
    elif aArgs.sAction == "page":
        pData = aArgs.sPageId.encode("utf-8")

    print(SendRequest(aArgs.host, aArgs.port, aArgs.token, sPath, sMethod, pData))


if __name__ == "__main__":
    Main()
