# Agent Guidelines

## GitHub CLI for Pipeline Checks

Use `gh` CLI to check CI workflow runs instead of opening a browser:

```bash
# List recent workflow runs
gh run list --repo githendrik/eink-desk-panel --limit 5

# Watch a specific run in real-time
gh run watch <RUN_ID> --repo githendrik/eink-desk-panel

# View run logs
gh run view <RUN_ID> --repo githendrik/eink-desk-panel --log

# Check if a run succeeded
gh run view <RUN_ID> --repo githendrik/eink-desk-panel --json status --jq '.status'
```

After pushing a tag (e.g., `v0.2.6`), wait ~2-3 minutes then verify:

```bash
gh run list --repo githendrik/eink-desk-panel --workflow build --branch v0.2.6 --limit 1
```

## Other Common Commands

```bash
# Check release exists
gh release view v0.2.6 --repo githendrik/eink-desk-panel

# List releases
gh release list --repo githendrik/eink-desk-panel
```
