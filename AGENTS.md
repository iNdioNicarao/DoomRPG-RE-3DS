# Agent Directives & Strict Rules

## 1. Absolute Prohibition: No Unprompted Git Push or GitHub Release Updates
- **NEVER** run `git push` to origin, remote, or any branch unless the user explicitly types a direct instruction to push (e.g. "push to origin", "push to github").
- **NEVER** run `gh release upload`, `gh release create`, or modify remote GitHub releases autonomously.
- Commits and builds must remain strictly local on this machine until the user gives explicit, unmistakable instruction to push.

## 2. Developer Attribution
- The development team must ALWAYS be set to **Dennis Isaac Gutierrez Zeledon (Dennis)**, NOT Arden Peacock.
