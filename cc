provider "aws" {
  region = "ap-south-1"
}

locals {
  domain_name = "terraform-aws-modules.modules.tf"
}

################################################################################
# The terraform_remote_state Data Source
################################################################################
data "terraform_remote_state" "vpc" {
  backend = "s3"

  config = {
    bucket = "kotak811-prod-terraform-state"
    key    = "kotak811/env/kotak811-prod/vpc/vpc.tfstate"
    region = "ap-south-1"
  }
}



#################################################################
# ALB Security Group
#################################################################

module "security_group" {
  source = "../../../../terraform-modules/sg"

  name        = "crossell-prod-internal-alb-sg"
  description = "Security group for onb-internal ALB SG"
  vpc_id      = data.terraform_remote_state.vpc.outputs.vpc_id

  use_name_prefix        = false
  revoke_rules_on_delete = true

  egress_rules = ["all-all"]
  ingress_with_cidr_blocks = [
    {
      from_port   = 443
      to_port     = 443
      protocol    = "tcp"
      description = "DEVOPS-1547 - HTTPS"
      cidr_blocks = "0.0.0.0/0"
    },
    {
      from_port   = 80
      to_port     = 80
      protocol    = "tcp"
      description = "DEVOPS-1547 - HTTP"
      cidr_blocks = "10.0.0.0/8"
    }
  ]
  tags = {
    entity               = "KMBL"
    application-name     = "811_AWS_CROSS_SELL"
    application-id       = "APP-01797"
    application-owner    = "Ranjit Mohan"
    application-manager  = "Imran Ansari"
    vertical-tlt         = "Ranjit Mohan"
    application-rating   = "high"
    ticket-id            = "DEVOPS-1547"
    environment          = "production"
    project-name         = "811-Crossell"
    budget-type          = "rtb"
    monitoring           = "yes"
    db-engine            = "NA"
    created-date         = "02-28-2024"
    instance-node-type   = "NA"
    backup               = "NA"
    scheduler-start      = "NA"
    scheduler-stop       = "NA"
    end-date             = "NA"
    created-by-terraform = "yes"
    terraform-version    = "1.8.4"
    resource-name        = "ec2-crossell-prod-tg"
    project-811          = "CROSSELL"
    db-engine-version    = "NA"
  }
  # ingress_cidr_blocks = ["0.0.0.0/0"]
  # ingress_rules       = ["http-80-tcp", "https-443-tcp"]
  # egress_rules        = ["all-all"]
}


##################################################################
# Application Load Balancer
##################################################################

module "alb" {
  source = "../../../../terraform-modules/alb"

  name = "k811-crossell-prod-internal-alb"

  load_balancer_type = "application"

  vpc_id                     = data.terraform_remote_state.vpc.outputs.vpc_id
  security_groups            = [module.security_group.security_group_id]
  subnets                    = slice(data.terraform_remote_state.vpc.outputs.private_subnets, 3, 6)
  internal                   = true
  enable_deletion_protection = true

  #   # See notes in README (ref: https://github.com/terraform-providers/terraform-provider-aws/issues/7987)
  #   access_logs = {
  #     bucket = module.log_bucket.s3_bucket_id
  #   }

  access_logs = {
    enabled = true
    bucket  = "aws-observability-logs-e569c200"
    prefix  = "k811-crossell-prod-internal-alb-logs"
  }

  http_tcp_listeners = [
    {
      port               = 80
      protocol           = "HTTP"
      target_group_index = 0
      # action_type        = "redirect"
      # redirect  = {
      #   port        = "443"
      #   protocol    = "HTTPS"
      #   status_code = "HTTP_301"
      # }
    }
  ]

  http_tcp_listener_rules = [
    {
      http_listener_index = 0
      priority            = 1
      actions = [{
        type               = "forward"
        target_group_index = 0
      }]
      conditions = [{
        host_headers = ["passbook-internal.kotak811.com"]
      }]
    }
  ]

  https_listeners = [
    {
      port               = 443
      protocol           = "HTTPS"
      ssl_policy         = "ELBSecurityPolicy-TLS-1-2-2017-01"
      certificate_arn    = "arn:aws:acm:ap-south-1:718378052708:certificate/b1a2167d-2ef8-4d9d-9e04-692d07b089b3"
      target_group_index = 0
    }
  ]

  https_listener_rules = [
    {
      https_listener_index = 0
      priority             = 1
      actions = [{
        type               = "forward"
        target_group_index = 0
      }]
      conditions = [{
        host_headers = ["passbook.kotak811.com"]
      }]
    },
    {
      https_listener_index = 0
      priority             = 2
      actions = [{
        type               = "forward"
        target_group_index = 0
      }]
      conditions = [{
        host_headers = ["passbook.kotak811.com"]
      }]
    },
    {
      https_listener_index = 0
      priority             = 3
      actions = [{
        type               = "forward"
        target_group_index = 0
      }]
      conditions = [{
        host_headers = ["passbook-internal.kotak811.com"]
      }]
    }
  ]

  target_groups = [
    {
      name = "crossell-prod-internal-alb-tg"
      #      name_prefix          = "prod-onb-"
      backend_protocol     = "HTTP"
      backend_port         = 30712
      target_type          = "instance"
      deregistration_delay = 10
      health_check = {
        enabled             = true
        interval            = 30
        path                = "/healthz"
        port                = "traffic-port"
        healthy_threshold   = 3
        unhealthy_threshold = 3
        timeout             = 6
        protocol            = "HTTP"
        matcher             = "200-399"
      }
      tags = {
        entity               = "KMBL"
        application-name     = "811_AWS_CROSS_SELL"
        application-id       = "APP-01797"
        application-owner    = "Ranjit Mohan"
        application-manager  = "Imran Ansari"
        vertical-tlt         = "Ranjit Mohan"
        application-rating   = "high"
        ticket-id            = "DEVOPS-1547"
        environment          = "production"
        project-name         = "811-Crossell"
        budget-type          = "rtb"
        monitoring           = "yes"
        db-engine            = "NA"
        created-date         = "02-28-2024"
        instance-node-type   = "NA"
        backup               = "NA"
        scheduler-start      = "NA"
        scheduler-stop       = "NA"
        end-date             = "NA"
        created-by-terraform = "yes"
        terraform-version    = "1.4.6"
        resource-name        = "ec2-crossell-prod-tg"
        project-811          = "CROSSELL"
        db-engine-version    = "NA"
      }
    },
    # {
    #   name                 = "passbook-prod-internal-alb-tg"
    #   backend_protocol     = "HTTP"
    #   backend_port         = 30712
    #   target_type          = "instance"
    #   deregistration_delay = 10
    #   health_check = {
    #     enabled             = true
    #     interval            = 30
    #     path                = "/healthz"
    #     port                = "traffic-port"
    #     healthy_threshold   = 3
    #     unhealthy_threshold = 3
    #     timeout             = 6
    #     protocol            = "HTTP"
    #     matcher             = "200-399"
    #   }
    #   tags = {
    #     entity               = "KMBL"
    #     application-name     = "811_AWS_CROSS_SELL"
    #     application-id       = "APP-01797"
    #     application-owner    = "Ranjit Mohan"
    #     application-manager  = "Imran Ansari"
    #     vertical-tlt         = "Ranjit Mohan"
    #     application-rating   = "high"
    #     ticket-id            = "DEVOPS-4317"
    #     environment          = "production"
    #     project-name         = "811-Crossell"
    #     budget-type          = "rtb"
    #     monitoring           = "yes"
    #     db-engine            = "NA"
    #     created-date         = "09-13-2024"
    #     instance-node-type   = "NA"
    #     backup               = "NA"
    #     scheduler-start      = "NA"
    #     scheduler-stop       = "NA"
    #     end-date             = "NA"
    #     created-by-terraform = "yes"
    #     terraform-version    = "1.8.4"
    #     resource-name        = "ec2-crossell-prod-tg"
    #     project-811          = "CROSSELL"
    #     db-engine-version    = "NA"
    #   }
    # }
  ]

  tags = {
    entity               = "KMBL"
    application-name     = "811_AWS_CROSS_SELL"
    application-id       = "APP-01797"
    application-owner    = "Ranjit Mohan"
    application-manager  = "Imran Ansari"
    vertical-tlt         = "Ranjit Mohan"
    application-rating   = "high"
    ticket-id            = "DEVOPS-1547"
    environment          = "production"
    project-name         = "811-Crossell"
    budget-type          = "rtb"
    monitoring           = "yes"
    db-engine            = "NA"
    created-date         = "02-28-2024"
    instance-node-type   = "NA"
    backup               = "NA"
    scheduler-start      = "NA"
    scheduler-stop       = "NA"
    end-date             = "NA"
    created-by-terraform = "yes"
    terraform-version    = "1.4.6"
    resource-name        = "elb-crossell-prod-alb"
    project-811          = "CROSSELL"
    db-engine-version    = "NA"
  }

}


###############################################################
#Backend Terraform State
###############################################################
terraform {
  backend "s3" {
    #Replace this with your bucket name!
    bucket = "kotak811-prod-terraform-state"
    key    = "kotak811/env/kotak811-prod/alb/crossell-alb-internal/alb.tfstate"
    region = "ap-south-1"
    #Replace this with your DynamoDB table name!
    dynamodb_table = "tf-up-and-run-locks"
    encrypt        = true
  }
}
