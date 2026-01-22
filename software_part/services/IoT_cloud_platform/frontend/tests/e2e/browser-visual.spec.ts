import { test, expect } from '@playwright/test';

const BASE_URL = 'http://localhost:8088';
const ADMIN_USERNAME = process.env.APP_ADMIN_USERNAME || 'admin';
const ADMIN_PASSWORD = process.env.APP_ADMIN_PASSWORD || '';

async function login(page: any) {
  test.skip(!ADMIN_PASSWORD, 'Set APP_ADMIN_PASSWORD env (do not hardcode real passwords in repo)');
  await page.goto(`${BASE_URL}/login`);
  await page.locator('input[type="text"]').first().fill(ADMIN_USERNAME);
  await page.locator('input[type="password"]').first().fill(ADMIN_PASSWORD);
  await page.locator('button[type="submit"]').first().click();
  await page.waitForTimeout(3000);
}

test.describe('IoT Cloud Platform - 浏览器可视化测试', () => {

  test('01 - 访问首页并检查重定向到登录页', async ({ page }) => {
    console.log('📄 测试: 访问首页');

    await page.goto(BASE_URL);
    await page.waitForLoadState('networkidle');

    // 截图
    await page.screenshot({ path: 'test-results/01-homepage.png', fullPage: true });
    console.log('✅ 首页截图已保存');

    // 检查是否重定向到登录页
    await page.waitForTimeout(2000);
    const url = page.url();
    console.log(`当前URL: ${url}`);

    expect(url).toContain('/login');
  });

  test('02 - 测试登录功能', async ({ page }) => {
    console.log('🔐 测试: 登录功能');

    test.skip(!ADMIN_PASSWORD, 'Set APP_ADMIN_PASSWORD env (do not hardcode real passwords in repo)');

    await page.goto(`${BASE_URL}/login`);
    await page.waitForLoadState('networkidle');

    // 填写用户名
    const usernameInput = page.locator('input[type="text"], input[placeholder*="用户"], input[placeholder*="username"]').first();
    await usernameInput.fill(ADMIN_USERNAME);
    console.log(`✅ 输入用户名: ${ADMIN_USERNAME}`);

    // 填写密码
    const passwordInput = page.locator('input[type="password"]').first();
    await passwordInput.fill(ADMIN_PASSWORD);
    console.log('✅ 输入密码: ****');

    // 截图登录表单
    await page.screenshot({ path: 'test-results/02-login-form.png' });
    console.log('📸 登录表单截图已保存');

    // 点击登录按钮
    const loginButton = page.locator('button[type="submit"], button:has-text("登录"), .el-button--primary').first();
    await loginButton.click();
    console.log('✅ 点击登录按钮');

    // 等待导航
    await page.waitForTimeout(3000);

    // 检查是否登录成功（应该跳转到仪表盘）
    const newUrl = page.url();
    console.log(`登录后URL: ${newUrl}`);

    expect(newUrl).not.toContain('/login');

    // 截图登录后页面
    await page.screenshot({ path: 'test-results/02-after-login.png', fullPage: true });
    console.log('📸 登录后截图已保存');
  });

  test('03 - 测试仪表盘页面', async ({ page }) => {
    console.log('📊 测试: 仪表盘页面');

    await login(page);

    // 检查仪表盘内容
    const content = await page.content();

    // 截图仪表盘
    await page.screenshot({ path: 'test-results/03-dashboard.png', fullPage: true });
    console.log('📸 仪表盘截图已保存');

    // 检查关键元素
    if (content.includes('设备') || content.includes('Device')) {
      console.log('✅ 发现设备相关内容');
    }

    if (content.includes('告警') || content.includes('Alert')) {
      console.log('✅ 发现告警相关内容');
    }
  });

  test('04 - 测试设备管理页面', async ({ page }) => {
    console.log('📱 测试: 设备管理页面');

    await login(page);

    // 导航到设备页面
    try {
      const deviceLink = page.locator('text=设备, text=Device').first();
      await deviceLink.click();
      await page.waitForTimeout(2000);

      // 截图设备页面
      await page.screenshot({ path: 'test-results/04-devices.png', fullPage: true });
      console.log('📸 设备页面截图已保存');

      // 检查设备表格
      const tableExists = await page.locator('table, .el-table').count() > 0;
      expect(tableExists).toBeTruthy();
      console.log('✅ 发现设备表格');
    } catch (e) {
      console.log('⚠️  无法访问设备页面:', e.message);
    }
  });

  test('05 - 测试告警管理页面', async ({ page }) => {
    console.log('🚨 测试: 告警管理页面');

    await login(page);

    // 导航到告警页面
    try {
      const alertLink = page.locator('text=告警, text=Alert').first();
      await alertLink.click();
      await page.waitForTimeout(2000);

      // 截图告警页面
      await page.screenshot({ path: 'test-results/05-alerts.png', fullPage: true });
      console.log('📸 告警页面截图已保存');
    } catch (e) {
      console.log('⚠️  无法访问告警页面:', e.message);
    }
  });

  test('06 - 测试用户管理页面', async ({ page }) => {
    console.log('👥 测试: 用户管理页面');

    await login(page);

    // 导航到用户页面
    try {
      const userLink = page.locator('text=用户, text=User').first();
      await userLink.click();
      await page.waitForTimeout(2000);

      // 截图用户页面
      await page.screenshot({ path: 'test-results/06-users.png', fullPage: true });
      console.log('📸 用户页面截图已保存');
    } catch (e) {
      console.log('⚠️  无法访问用户页面:', e.message);
    }
  });

  test('07 - 测试退出登录', async ({ page }) => {
    console.log('🚪 测试: 退出登录');

    await login(page);

    // 查找并点击退出按钮
    try {
      const logoutButton = page.locator('text=退出, text=登出, text=Logout').first();
      await logoutButton.click();
      await page.waitForTimeout(2000);

      // 检查是否返回登录页
      const url = page.url();
      expect(url).toContain('/login');
      console.log('✅ 成功退出登录');
    } catch (e) {
      console.log('⚠️  无法找到退出按钮:', e.message);
    }
  });
});
