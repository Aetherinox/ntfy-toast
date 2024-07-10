<div align="center">
<h1>♾️ ntfy-toast ♾️</h1>
<br />

<p>

ntfy-toast is a forked verison of [SnoreToast](https://github.com/KDE/snoretoast) which has been modified to fit a certain list of criteria, including notifications that do not time out.

<br />

This library is packaged with [ntfy-desktop](https://github.com/Aetherinox/ntfy-desktop)
</p>

<br />

<img src="https://github.com/Aetherinox/ntfy-desktop/assets/118329232/cd7dca36-e0cc-43dc-a4c9-c09e084b3cd0" width="630">

<br />

</div>

<br />

<div align="center">

<!-- prettier-ignore-start -->
[![Version][badge-version-gh]][link-version-gh] [![Build Status][badge-build]][link-build] [![Downloads][badge-downloads-gh]][link-downloads-gh] [![Size][badge-size-gh]][badge-size-gh] [![Last Commit][badge-commit]][badge-commit]
<!-- prettier-ignore-end -->

</div>

<br />

---

<br />

- [About](#about)
  - [What is ntfy?](#what-is-ntfy)
  - [What is ntfy-desktop](#what-is-ntfy-desktop)
  - [What is ntfy-toast](#what-is-ntfy-toast)
- [Features](#features)
- [Usage](#usage)
- [CLI Arguments](#cli-arguments)
- [Build](#build)


<br />

---

<br />

# About
Getting familiar with the projects:

<br />

## What is ntfy?
[ntfy.sh](https://ntfy/) (pronounced "notify") is a simple HTTP-based pub-sub notification service. With ntfy, you can send notifications to your phone or desktop via scripts from any computer, without having to sign up or pay any fees. If you'd like to run your own instance of the service, you can easily do so since ntfy is open source.

<br />

<div align="center">

[![View](https://img.shields.io/badge/%20-%20View%20Project%20Repo-%20%23de2343?style=for-the-badge&logo=github&logoColor=FFFFFF)](https://github.com/binwiederhier/ntfy)

</div>

<br />

## What is ntfy-desktop
[ntfy-desktop](https://github.com/Aetherinox/ntfy-desktop) allows you to access the official free / paid notification service [ntfy.sh](https://ntfy.sh/), or your own self-hosted version of ntfy from within a desktop application which utilizes Electron as the wrapper.

This version of ntfy-desktop is based on the package ntfy-electron created by xdpirate, however, this version brings some changes in functionality, as well as some additional edits that I personally needed.

<div align="center">

[![View](https://img.shields.io/badge/%20-%20View%20Project%20Repo-%20%23de2343?style=for-the-badge&logo=github&logoColor=FFFFFF)](https://github.com/Aetherinox/ntfy-desktop)

</div>

<br />

## What is ntfy-toast
[ntfy-toast](https://github.com/Aetherinox/ntfy-toast) is a notification system for Windows 10/11 which is used within [ntfy-desktop](https://github.com/Aetherinox/ntfy-desktop) to display notifications for users.

It is based on [SnoreToast](https://github.com/KDE/snoretoast), but has been updated with numerous features.

<br />

---

<br />

# Features
- Single binary file which works with x32 and x64 bit operating systems
- New `-persistent` option which allows a notification to stay on a user's screen indefinitely until the user closes it.
- Many small bug fixes

<br />

---

<br />

# Usage
When using the command-line to show notifications, you can push a notification by opening Command Prompt and running:

```shell
cd X:\path\to\ntfy-toast\out\build\x64-Debug\bin
```

<br />

Then push a notification with:
```shell ignore
ntfy-toast.exe -t "Title" -m "Message" -persistent
```

<br />

---

<br />

# CLI Arguments

| Argument | Type | Description |
| --- | --- | --- |
| `-t` | `<title string>` | Title / first line of text in notification |
| `-m` | `<message string>` | Message displayed in notification |
| `-b` | `<btn1;bbtn2 string>` | Buttons <br /> List multiple buttons separated by `;` |
| `-tb` |  | Textbox on the bottom line, only if buttons are not specified |
| `-p` | `<image URI>` | Picture / image, local files only. |
| `-id` | `<id>` | Picture / image, local files only. |
| `-s` | `<sound URI>` | Sound when notification opened <br /><br /> [Possible options](http://msdn.microsoft.com/en-us/library/windows/apps/hh761492.aspx) |
| `-silent` |  | Disable playing sound when notification appears |
| `-persistent` |  | Force notification to stay on screen |
| `-d` | `short, long` | How long a notification stays on screen. <br /><br /> Only works if `-persistent` not specified. <br /><br /> Can only pick two options: <br />- `short` (7 seconds) <br /> - `long` (25 seconds) |
| `-appID` | `<App.ID>` | Don't create a shortcut but use the provided app id. |
| `-pid` | `<pid>` | Query the appid for the process <pid>, use -appID as fallback. (Only relevant for applications that might be packaged for the store |
| `-pipeName` | `<\.\pipe\pipeName\>` | Name pipe which is used for callbacks |
| `-application` | `<C:\foo\bar.exe>` | App to start if the pipe does not exist |
| `-close` | `<id>` | Close an existing notification |

<br />

---

<br />

# Build
This easiest way to build this package is to download the source and place it somewhere on your system.

Install Visual Studio and launch the application. Then go to **File** -> **Open** -> **Folder**.

<br />

<div align="center">

<img src="https://github.com/Aetherinox/ntfy-desktop/assets/118329232/9f91613f-5104-4c30-a623-5ea632a12ed9" width="630">

</div>

<br />

Select the folder where you downloaded ntfy-toast and open it. After a few moments, a few new folders will be created:

- 📁 .vs
- 📁 out

<br />

You can now start writing your code. Once you are finished, you need to build the application in Visual Studio.

<br />

<div align="center">

<img src="https://github.com/Aetherinox/ntfy-desktop/assets/118329232/4a045304-7296-4488-8317-b72772aa97fb" width="330">

</div>

<br />

Your binary will be built and placed inside:
- `project-folder/out/build/x64-Debug/bin/ntfy-toast.exe`

<br />

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->
<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->
<!-- ALL-CONTRIBUTORS-LIST:END -->

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

<br />
<br />

<!-- prettier-ignore-start -->
<!-- BADGE > GENERAL -->
[link-general-npm]: https://npmjs.com
[link-general-nodejs]: https://nodejs.org
[link-npmtrends]: http://npmtrends.com/ntfy-toast
<!-- BADGE > VERSION > GITHUB -->
[badge-version-gh]: https://img.shields.io/github/v/tag/Aetherinox/ntfy-toast?logo=GitHub&label=Version&color=ba5225
[link-version-gh]: https://github.com/Aetherinox/ntfy-toast/releases
<!-- BADGE > VERSION > NPMJS -->
[badge-version-npm]: https://img.shields.io/npm/v/ntfy-toast?logo=npm&label=Version&color=ba5225
[link-version-npm]: https://npmjs.com/package/ntfy-toast
<!-- BADGE > LICENSE -->
[badge-license-mit]: https://img.shields.io/badge/MIT-FFF?logo=creativecommons&logoColor=FFFFFF&label=License&color=9d29a0
[link-license-mit]: https://github.com/Aetherinox/ntfy-toast/blob/main/LICENSE
<!-- BADGE > BUILD -->
[badge-build]: https://img.shields.io/github/actions/workflow/status/Aetherinox/ntfy-toast/release-npm.yml?logo=github&logoColor=FFFFFF&label=Build&color=%23278b30
[link-build]: https://github.com/Aetherinox/ntfy-toast/actions/workflows/release-npm.yml
<!-- BADGE > DOWNLOAD COUNT -->
[badge-downloads-gh]: https://img.shields.io/github/downloads/Aetherinox/ntfy-toast/total?logo=github&logoColor=FFFFFF&label=Downloads&color=376892
[link-downloads-gh]: https://github.com/Aetherinox/ntfy-toast/releases
[badge-downloads-npm]: https://img.shields.io/npm/dw/%40aetherinox%2Fmarked-alert-fa?logo=npm&&label=Downloads&color=376892
[link-downloads-npm]: https://npmjs.com/package/ntfy-toast
<!-- BADGE > DOWNLOAD SIZE -->
[badge-size-gh]: https://img.shields.io/github/repo-size/Aetherinox/ntfy-toast?logo=github&label=Size&color=59702a
[link-size-gh]: https://github.com/Aetherinox/ntfy-toast/releases
[badge-size-npm]: https://img.shields.io/npm/unpacked-size/ntfy-toast/latest?logo=npm&label=Size&color=59702a
[link-size-npm]: https://npmjs.com/package/ntfy-toast
<!-- BADGE > COVERAGE -->
[badge-coverage]: https://img.shields.io/codecov/c/github/Aetherinox/ntfy-toast?token=MPAVASGIOG&logo=codecov&logoColor=FFFFFF&label=Coverage&color=354b9e
[link-coverage]: https://codecov.io/github/Aetherinox/ntfy-toast
<!-- BADGE > ALL CONTRIBUTORS -->
[badge-all-contributors]: https://img.shields.io/github/all-contributors/Aetherinox/ntfy-toast?logo=contributorcovenant&color=de1f6f&label=contributors
[link-all-contributors]: https://github.com/all-contributors/all-contributors
[badge-tests]: https://img.shields.io/github/actions/workflow/status/Aetherinox/marked-alert-fa/npm-tests.yml?logo=github&label=Tests&color=2c6488
[link-tests]: https://github.com/Aetherinox/ntfy-toast/actions/workflows/tests.yml
[badge-commit]: https://img.shields.io/github/last-commit/Aetherinox/ntfy-toast?logo=conventionalcommits&logoColor=FFFFFF&label=Last%20Commit&color=313131
[link-commit]: https://github.com/Aetherinox/ntfy-toast/commits/main/
<!-- prettier-ignore-end -->