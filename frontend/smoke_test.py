from pathlib import Path
import os
from playwright.sync_api import sync_playwright


with sync_playwright() as playwright:
    base_url = os.environ.get("LOCALRAG_TEST_URL", "http://127.0.0.1:4186")
    browser = playwright.chromium.launch(headless=True)
    desktop = browser.new_page(viewport={"width": 1440, "height": 960}, device_scale_factor=1)
    desktop.goto(base_url, wait_until="networkidle")
    desktop.locator(".app-shell").wait_for()

    history_search = desktop.get_by_role("textbox", name="搜索历史问题")
    composer = desktop.get_by_role("textbox", name="问题输入框")
    composer.fill("新问题用于搜索验证")
    desktop.get_by_role("button", name="发送问题").click()
    desktop.locator(".message.user").filter(has_text="新问题用于搜索验证").wait_for()
    history_search.fill("搜索验证")
    assert desktop.locator(".history-item").count() == 1
    assert "新问题用于搜索验证" in desktop.locator(".history-item").inner_text()
    history_search.fill("完全不存在的问题")
    desktop.get_by_text("没有找到相关问题", exact=True).wait_for()
    desktop.get_by_role("button", name="清除搜索").click()
    assert desktop.locator(".history-item").count() == 1
    history_search.press("Escape")
    assert desktop.locator(".history-item").count() == 1
    composer.press("Control+k")
    assert history_search.evaluate("element => document.activeElement === element")
    desktop.locator(".settings-trigger").click()
    desktop.locator(".settings-panel").wait_for()
    desktop.get_by_role("button", name="清空本地库").wait_for()
    file_input = desktop.locator("input[type='file'][aria-label='导入本地资料']")
    file_input.wait_for()
    assert ".docx" in (file_input.get_attribute("accept") or "")
    desktop.locator(".folder-cover").click()
    desktop.locator(".document-delete").first.wait_for()
    desktop.locator(".folder-document").first.click(button="right")
    desktop.get_by_role("menuitem", name="删除资料").wait_for()
    desktop.get_by_label("API Key（仅本机）").wait_for()
    assert desktop.locator("text=4 GB").count() == 0
    desktop.screenshot(path=str(Path("E:/codex/localrag-real-settings.png")), full_page=True)
    desktop.get_by_role("button", name="关闭设置").click()

    stats_response = desktop.evaluate("""async () => {
        const response = await fetch('/api/localrag/stats');
        return { ok: response.ok, body: await response.json() };
    }""")
    assert stats_response["ok"]
    stats = stats_response["body"]["stats"]
    assert set(stats) == {"documents", "chunks", "messages", "retrievalLogs"}

    chat_response = desktop.evaluate("""async () => {
        const response = await fetch('/api/chat', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({question: '测试本地 API'})
        });
        return { status: response.status, body: await response.json() };
    }""")
    assert chat_response["status"] == 400
    assert "API Key" in chat_response["body"]["error"]

    delete_response = desktop.evaluate("""async () => {
        const response = await fetch('/api/localrag/delete', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({id: 'local-999999999'})
        });
        return {status: response.status, body: await response.json()};
    }""")
    assert delete_response["status"] == 404

    open_response = desktop.evaluate("""async () => {
        const response = await fetch('/api/localrag/open', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({id: 'local-999999999'})
        });
        return {status: response.status, body: await response.json()};
    }""")
    assert open_response["status"] == 404

    mobile = browser.new_page(viewport={"width": 390, "height": 844}, device_scale_factor=1)
    mobile.goto(base_url, wait_until="networkidle")
    mobile.locator(".mobile-menu-button").click()
    mobile.locator(".sidebar.mobile-open").wait_for()
    mobile.locator(".settings-trigger").click()
    mobile.locator(".settings-panel").wait_for()
    print("smoke checks passed")
    browser.close()
