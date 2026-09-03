---
trigger: always_on
description: Strict prohibition against pushing to git remotes or GitHub without explicit user instructions
---

# Git Push and Remote Policy

1. **NEVER PUSH TO GITHUB WITHOUT EXPLICIT INSTRUCTION**:
   - Do NOT run `git push` under any circumstance unless the user has explicitly requested it in that exact prompt.
   - Do NOT run `gh release create`, `gh release upload`, or update GitHub releases autonomously.
   - All builds, tags, and commits must remain strictly local.
