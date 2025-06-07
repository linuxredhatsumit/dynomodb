
trigger: none

variables:
  - name: ENV
    value: uat
  - name: SERVICE_NAME
    value: k811-ms-cc-limit-correct-cronjob
  # Pool details
  - name: POOL_NAME
    value: "K811-DevOps"
  # For AWS Role Access
  - name: AWS_ACCOUNT_ID
    value: "483584640083"
  - name: ROLE_NAME
    value: EKS_Setup_Role
  # Helm related variables
  - name: HELM_CHARTS_PATH
    value: helm-charts/k811-ms-cc-limit-correct-cronjob/charts
  - name: HELM_S3BUCKET_URL
    value: s3://kotak811-helmcharts/uat/k811-ms-cc-limit-correct-cronjob/
  - name: KUBE_CONFIG_PATH
    value: /home/app_user/.kube/config-uat
  - name: NAMESPACE
    value: 811-uat
  - group: UAT-Static-Variables
  - name: DR_HELM_S3BUCKET_URL
    value: s3://kotak811-helmcharts-dr/uat/k811-ms-cc-limit-correct-cronjob/
  - name: DR_KUBE_CONFIG_PATH
    value: /home/app_user/.kube/config-uat-dr
  - name: DR_NAMESPACE
    value: 811-uat-dr
  # UAT ECR details
  - name: AWS_REGION
    value: ap-south-1
  - name: ECR_FOLDER_NAME
    value: 811uatonb
  - name: ECR_REPO_NAME
    value: k811_ms_cc_limit_correct_cronjob
  # DR ECR details
  - name: DR_AWS_REGION
    value: ap-south-2
  - name: DR_ECR_FOLDER_NAME
    value: 811uatonbdr
  - name: DR_ECR_REPO_NAME
    value: k811_ms_cc_limit_correct_cronjobdr

parameters:
  - name: DeployUAT
    displayName: "Deploy to UAT"
    type: boolean
    default: true
  - name: DeployDR
    displayName: "Deploy to DR"
    type: boolean
    default: false

pool:
  name: $(POOL_NAME)

stages:
  - template: build.yaml
    parameters:
      DeployUAT: ${{ parameters.DeployUAT }}
      service: $(SERVICE_NAME)

  - template: dr-build.yaml
    parameters:
      DeployDR: ${{ parameters.DeployDR }}
      service: $(SERVICE_NAME)
