# Initialise Media Foundation

A step-by-step guide to creating a minimal **Media Foundation "Hello World"** project in C++ using Visual Studio.

---

## 1️⃣ Create the Project

1. Open **Visual Studio**.
2. Go to **File → New → Project**.
3. Select **Console App (C++)**:
   - On Visual Studio 2022, search for “Console App” and pick **C++**.
4. Click **Next**.
5. Name your project (e.g., `HelloMediaFoundation`).
6. Choose a **location** and select the **Windows SDK version** (latest installed, e.g., 10 or 11).
7. Click **Create**.

This creates a simple C++ console application with a `main()` function — perfect for learning Media Foundation.

---

## 2️⃣ Add Media Foundation Dependencies

By default, Visual Studio includes the Windows headers, but you need to **link the Media Foundation libraries**.

1. Right-click your project → **Properties**.
2. Go to **Linker → Input → Additional Dependencies**.
3. Add the following libraries:
   ---
   Mfplat.lib; Mfreadwrite.lib; Mf.lib; Mfuuid.lib; Strmiids.lib; Ole32.lib
   ---
