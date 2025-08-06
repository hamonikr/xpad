# CLAUDE.md

name: "Intelligent PR Automation Pipeline"
description: |
  End-to-end automated pipeline for pull request review, testing, and deployment using Claude-powered agents.
  Designed for full automation (no user prompts) and clear task separation across sub agents.

auto: true   # Global setting: All steps run without user confirmation

agents:
  - name: "StaticAnalysisAgent"
    role: "Perform static code analysis on incoming PRs"
    input: "pull_request.diff"
    output: "static_analysis_report.md"
    auto: true
    model: "claude-3-sonnet"
    policies:
      - "Fail build if critical issues detected"
      - "Report all warnings to review log"
    settings:
      language: "python"
      tools: ["flake8", "bandit"]

  - name: "UnitTestAgent"
    role: "Run automated unit tests and summarize results"
    input: "static_analysis_report.md"
    output: "unit_test_results.json"
    auto: true
    model: "claude-3-sonnet"
    policies:
      - "Block deploy if tests fail"
      - "Capture full logs"
    settings:
      framework: "pytest"
      coverage: true

  - name: "SecurityScanAgent"
    role: "Scan code and dependencies for security vulnerabilities"
    input: "pull_request.diff"
    output: "security_report.md"
    auto: true
    model: "claude-3-opus"
    policies:
      - "Block release on high-severity CVEs"
    settings:
      scanner: "Snyk"
      severity_threshold: "high"

  - name: "DeployAgent"
    role: "Deploy code to staging (or production) upon passing tests"
    input: "unit_test_results.json"
    output: "deployment.log"
    auto: true
    model: "claude-3-sonnet"
    policies:
      - "Rollback on deployment failure"
      - "Notify team via Slack"
    settings:
      environment: "staging"
      notify: true

policies:
  - "Each agent logs actions and outputs to central audit log"
  - "All failures trigger notification and rollback"
  - "Strict least-privilege principle for all credentials"
  - "All artifacts are stored with version tags"

settings:
  llm_model: "claude-3-sonnet"
  max_parallel_agents: 4
  notification_channel: "slack"
  logging_level: "verbose"
  default_timeout: 600

# End of CLAUDE.md

