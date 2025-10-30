
provider "aws" {
    region = local.region
}

locals {
    name = "k811-onb-dev-eks-admin-sa"
    region = "ap-south-1"
}
###############################
# IAM Role for onb-dev SA
###############################
module "eks_sa_iam_role" {
    source = "../../../../../terraform-modules/iam/modules/iam-assumable-role-with-oidc"

    create_role = true

    role_name = local.name

    tags = {
        entity               = "KMBL"
        application-name     = "811 Onboarding App"
        application-id       = "APP-01783"
        application-owner    = "Amlan Mandal"
        application-manager  = "Anilkumar singam"
        vertical-tlt         = "Manish A"
        application-rating   = "high"
        ticket-id            = "DEVOPS-1172 DEVOPS-3056"
        environment          = "dev"
        project-name         = "811-ONB"
        Name                 = "k811-onb-dev-eks-admin-sa"
        budget-type          = "rtb"
        monitoring           = "yes"
        db-engine            = "NA"
        created-date         = "11-21-2023"
        instance-node-type   = "NA"
        backup               = "no"
        scheduler-start      = "NA"
        scheduler-stop       = "NA"
        end-date             = "NA"
        created-by-terraform = "yes"
        terraform-version    = "1.4.6"
        resource-name        =  "iam-onb-admin-role"
        project-811          = "ONB"
    }

    #   provider_url  = "oidc.eks.ap-south-1.amazonaws.com/id/ECF3C103D341F9CA41520CDD582CB807"
    provider_urls = ["oidc.eks.ap-south-1.amazonaws.com/id/ECF3C103D341F9CA41520CDD582CB807","oidc.eks.ap-south-1.amazonaws.com/id/0D04991F1AA697F7FD8125ABFBEE2857"]

    role_policy_arns = [
    module.iam_policy.arn,
    "arn:aws:iam::aws:policy/AmazonCognitoPowerUser"
    ]

    oidc_fully_qualified_subjects = ["system:serviceaccount:dev:811-onb-dev-sa-admin"]
}

#########################################
# IAM policy
#########################################
module "iam_policy" {
    source = "../../../../../terraform-modules/iam/modules/iam-policy"

    name        = "${local.name}-policy"
    path        = "/"
    description = "Customized policy for the ${local.name} to access aws services" 

    policy = <<EOF
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "ssm:GetParameters",
                "ssm:PutParameter",
                "ssm:GetParameter"
            ],
            "Resource": [
                "arn:aws:ssm:ap-south-1:483584640083:parameter/dev/811-ONB/k811-be-admin/*",
                "arn:aws:ssm:ap-south-1:483584640083:parameter/dev/811-ONB/COMMON_*"
            ]
        },
        {
            "Action": [
                "kms:Decrypt",
                "kms:Encrypt",
                "kms:GenerateDataKey"
            ],
            "Effect": "Allow",
            "Resource": [
                "arn:aws:kms:ap-south-1:483584640083:key/fd55a4ac-4afd-467f-b41d-ce6881f6cd61",
                "arn:aws:kms:ap-south-1:483584640083:key/cc498c37-f3b6-4dfa-a9fb-4e98996be115",
                "arn:aws:kms:ap-south-1:286714098649:key/d8bdefed-688c-496d-b438-1c955676fcca"
            ]
        },
        {
            "Effect": "Allow",
            "Action": [
                "ssm:DescribeParameters",
                "secretsmanager:DescribeSecret",
                "secretsmanager:GetSecretValue"
            ],
            "Resource": [
              "arn:aws:secretsmanager:ap-south-1:483584640083:secret:dev/istio_codec/symmetric-mIBuc8",
              "arn:aws:secretsmanager:ap-south-1:483584640083:secret:dev/istio_codec/hybrid-tUT4KP"
              ]
        },
        {
            "Effect": "Allow",
            "Action": [
                "s3:GetObject"
            ],
            "Resource": [
                "arn:aws:s3:::cloudacqui-backend-uat",
                "arn:aws:s3:::cloudacqui-backend-uat/*",
                "arn:aws:s3:::kotak-811-liveliness-check-dev",
                "arn:aws:s3:::kotak-811-liveliness-check-dev/*",
                "arn:aws:s3:::uat-vkyc-attachments",         
                "arn:aws:s3:::uat-vkyc-attachments/*"
            ]
        },
        {
            "Action": [
                "s3:GetObject",
                "s3:PutObject",
                "s3:ListBucket"
            ],
            "Effect": "Allow",
            "Resource": [
                "arn:aws:s3:::rewards-proposal-dev",
                "arn:aws:s3:::rewards-proposal-dev/*",
                "arn:aws:s3:::reward-offer-config-dev",
                "arn:aws:s3:::reward-offer-config-dev/*",
                "arn:aws:s3:::s3-admin-be",
                "arn:aws:s3:::s3-admin-be/*",
                "arn:aws:s3:::811-scc-liquidation-failure-reports-dev",
                "arn:aws:s3:::811-scc-liquidation-failure-reports-dev/*",
                "arn:aws:s3:::hv-kotak-811-liveliness-check-dev",
                "arn:aws:s3:::hv-kotak-811-liveliness-check-dev/*
            ]
        },
        {
            "Effect": "Allow",
            "Action": [
                "lambda:InvokeFunction"
            ],
            "Resource": "arn:aws:lambda:ap-south-1:483584640083:function:811_lambda_rewards_process_offers"
        },
        {
            "Sid": "VisualEditor0",
            "Effect": "Allow",
            "Action": [
                "sns:ListTagsForResource",
                "sns:ListSubscriptionsByTopic",
                "sns:Publish",
                "sns:GetTopicAttributes",
                "sns:GetDataProtectionPolicy"
            ],
            "Resource": [
                "arn:aws:sns:ap-south-1:483584640083:NOTIFICATION_TOPIC_dev"
            ]
        },
        {
            "Effect": "Allow",
            "Action": [
                "dynamodb:Query",
                "dynamodb:GetItem",
                "dynamodb:ListContributorInsights",
                "dynamodb:ListGlobalTables",
                "dynamodb:ListTables",
                "dynamodb:ListBackups",
                "dynamodb:ListImports",
                "dynamodb:ListExports"
            ],
            "Resource": [
                "arn:aws:dynamodb:ap-south-1:483584640083:table/USER_PROFILE_dev*",
                "arn:aws:dynamodb:ap-south-1:483584640083:table/UPI_CUSTOMER_PROFILE_dev*",
                "arn:aws:dynamodb:ap-south-1:483584640083:table/REWARDS_JOURNEY_dev*",
                "arn:aws:dynamodb:ap-south-1:483584640083:table/CASHBACK_TRANSACTION_dev*",
                "arn:aws:dynamodb:ap-south-1:483584640083:table/REWARDS_EVENT_dev*",
                "arn:aws:dynamodb:ap-south-1:483584640083:table/REWARDS_dev*",
                "arn:aws:dynamodb:ap-south-1:483584640083:table/REWARDS_PROPOSAL_dev*"
            ]
        },
        {
            "Effect": "Allow",
            "Action": [
                "dynamodb:UpdateItem",
                "dynamodb:PutItem"
            ],
            "Resource": [
                "arn:aws:dynamodb:ap-south-1:483584640083:table/REWARDS_PROPOSAL_dev*"
            ]
        },
        {
            "Action": [
                "ses:GetAccount",
                "ses:List*"
            ],
            "Effect": "Allow",
            "Resource": "*"
        },
        {
            "Action": [
                "ses:SendRawEmail"
            ],
            "Effect": "Allow",
            "Resource": "arn:aws:ses:ap-south-1:483584640083:identity/kotak811.com"
        },
        {
            "Sid": "DEVOPS8020",
            "Effect": "Allow",
            "Action": [
                "s3:*"
            ],
            "Resource": [
                "arn:aws:s3:::s3-admin-be/media/*",
                "arn:aws:s3:::s3-admin-be/static/*"
            ]
        },
        {
            "Effect": "Allow",
            "Action": [
                "appconfig:ListTagsForResource",
                "appconfig:GetHostedConfigurationVersion",
                "appconfig:GetDeployment",
                "appconfig:ListEnvironments",
                "appconfig:StartDeployment",
                "appconfig:GetExtensionAssociation",
                "appconfig:GetLatestConfiguration",
                "appconfig:ListDeployments",
                "appconfig:GetExtension",
                "appconfig:GetEnvironment",
                "appconfig:GetDeploymentStrategy",
                "appconfig:ListConfigurationProfiles",
                "appconfig:GetConfiguration",
                "appconfig:GetApplication",
                "appconfig:GetConfigurationProfile",
                "appconfig:ListHostedConfigurationVersions",
                "appconfig:StartConfigurationSession"
            ],
            "Resource": [
                "arn:aws:appconfig:ap-south-1:483584640083:application/61i9ozu",
                "arn:aws:appconfig:ap-south-1:483584640083:application/61i9ozu/configurationprofile/*",
                "arn:aws:appconfig:ap-south-1:483584640083:application/61i9ozu/environment/*"
            ]
        }
    ]
}
EOF
}

###############################################################
#Backend Terraform State
###############################################################
terraform {
    backend "s3" {
        bucket            = "kotak811-terraform-state"
        key               = "kotak811/env/kotak811-dev/iam/onb-sa-roles/admin/iam.tfstate"
        region            = "ap-south-1"
        dynamodb_table    = "tf-up-and-run-locks"
        encrypt           = true
    }
}


below error


Planning failed. Terraform encountered an error while generating this plan.

╷
│ Error: "policy" contains an invalid JSON policy: invalid character '\n' in string literal, at byte offset 2771
│ 
│   with module.iam_policy.aws_iam_policy.policy[0],
│   on ../../../../../terraform-modules/iam/modules/iam-policy/main.tf line 8, in resource "aws_iam_policy" "policy":
│    8:   policy = var.policy
