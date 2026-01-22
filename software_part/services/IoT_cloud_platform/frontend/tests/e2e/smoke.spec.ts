import { test, expect } from '@playwright/test';

test('login page shows form', async ({ page }) => {
  await page.goto('/');
  await expect(page.locator('form')).toBeVisible({ timeout: 5000 });
  await expect(page.locator('text=登录')).toBeVisible();
});
