#!/usr/bin/env bash
# ==============================================================================
# MonolithV - Oracle Cloud Infrastructure (OCI) Compute Provisioning Script
# ==============================================================================
# This reference script documents the OCI CLI equivalent of the steps performed
# via the Oracle Cloud Console for Phase 1 (P1.6).
#
# Requirements:
#   - OCI CLI installed and configured (`oci setup config`)
#   - Appropriate IAM permissions in the target compartment
#   - Existing SSH public key file available locally
#
# Usage:
#   export COMPARTMENT_OCID="ocid1.compartment.oc1..xxxx"
#   export SSH_PUBLIC_KEY_PATH="$HOME/.ssh/id_rsa.pub"
#   ./provision-vm.sh
# ==============================================================================

set -euo pipefail

# --- Configuration & Defaults ---
COMPARTMENT_OCID="${COMPARTMENT_OCID:-}"
SSH_PUBLIC_KEY_PATH="${SSH_PUBLIC_KEY_PATH:-$HOME/.ssh/id_rsa.pub}"

# Shape & Resource Configuration (Always Free Eligible)
SHAPE="VM.Standard.A1.Flex"
OCPUS="2"
MEMORY_IN_GBS="12"
DISPLAY_NAME="MonolithV-Server"
VCN_NAME="MonolithV-VCN"
SUBNET_NAME="MonolithV-Public-Subnet"
CIDR_BLOCK="10.0.0.0/16"
SUBNET_CIDR_BLOCK="10.0.0.0/24"

# Validate required environment variables
if [[ -z "${COMPARTMENT_OCID}" ]]; then
  echo "ERROR: COMPARTMENT_OCID environment variable must be set."
  echo "Example: export COMPARTMENT_OCID=\"ocid1.compartment.oc1..xxxxx\""
  exit 1
fi

if [[ ! -f "${SSH_PUBLIC_KEY_PATH}" ]]; then
  echo "ERROR: SSH public key not found at ${SSH_PUBLIC_KEY_PATH}"
  exit 1
fi

echo "==================================================================="
echo " Starting OCI Provisioning Reference Flow for ${DISPLAY_NAME}..."
echo " Shape: ${SHAPE} (${OCPUS} OCPUs, ${MEMORY_IN_GBS} GB RAM)"
echo " Compartment OCID: ${COMPARTMENT_OCID}"
echo "==================================================================="

# 1. Get Canonical Ubuntu 22.04 LTS Image OCID for Ampere (A1.Flex)
echo "[1/4] Retrieving Ubuntu 22.04 LTS (aarch64) Image OCID..."
IMAGE_OCID=$(oci compute image list \
  --compartment-id "${COMPARTMENT_OCID}" \
  --operating-system "Canonical Ubuntu" \
  --operating-system-version "22.04" \
  --shape "${SHAPE}" \
  --sort-by "TIMECREATED" \
  --sort-order "DESC" \
  --query "data[0].id" \
  --raw-output)

if [[ -z "${IMAGE_OCID}" || "${IMAGE_OCID}" == "null" ]]; then
  echo "ERROR: Failed to find a valid Ubuntu 22.04 image for shape ${SHAPE}."
  exit 1
fi
echo "      Found Image OCID: ${IMAGE_OCID}"

# 2. Check or Create Virtual Cloud Network (VCN)
echo "[2/4] Checking Virtual Cloud Network (${VCN_NAME})..."
VCN_OCID=$(oci network vcn list \
  --compartment-id "${COMPARTMENT_OCID}" \
  --display-name "${VCN_NAME}" \
  --query "data[0].id" \
  --raw-output 2>/dev/null || true)

if [[ -z "${VCN_OCID}" || "${VCN_OCID}" == "null" ]]; then
  echo "      Creating VCN ${VCN_NAME} (${CIDR_BLOCK})..."
  VCN_OCID=$(oci network vcn create \
    --compartment-id "${COMPARTMENT_OCID}" \
    --display-name "${VCN_NAME}" \
    --cidr-block "${CIDR_BLOCK}" \
    --query "data.id" \
    --raw-output)
fi
echo "      VCN OCID: ${VCN_OCID}"

# 3. Check or Create Public Subnet
echo "[3/4] Checking Public Subnet (${SUBNET_NAME})..."
SUBNET_OCID=$(oci network subnet list \
  --compartment-id "${COMPARTMENT_OCID}" \
  --vcn-id "${VCN_OCID}" \
  --display-name "${SUBNET_NAME}" \
  --query "data[0].id" \
  --raw-output 2>/dev/null || true)

if [[ -z "${SUBNET_OCID}" || "${SUBNET_OCID}" == "null" ]]; then
  echo "      Creating Public Subnet ${SUBNET_NAME} (${SUBNET_CIDR_BLOCK})..."
  SUBNET_OCID=$(oci network subnet create \
    --compartment-id "${COMPARTMENT_OCID}" \
    --vcn-id "${VCN_OCID}" \
    --display-name "${SUBNET_NAME}" \
    --cidr-block "${SUBNET_CIDR_BLOCK}" \
    --query "data.id" \
    --raw-output)
fi
echo "      Subnet OCID: ${SUBNET_OCID}"

# 4. Launch Compute Instance
echo "[4/4] Launching Compute Instance (${DISPLAY_NAME})..."
echo "      Note: If ap-mumbai-1 is out of A1.Flex capacity, OCI will return a 404/500 Out of Capacity error."

INSTANCE_OCID=$(oci compute instance launch \
  --compartment-id "${COMPARTMENT_OCID}" \
  --availability-domain "$(oci iam availability-domain list --compartment-id "${COMPARTMENT_OCID}" --query "data[0].name" --raw-output)" \
  --shape "${SHAPE}" \
  --shape-config "{\"ocpus\": ${OCPUS}, \"memoryInGBs\": ${MEMORY_IN_GBS}}" \
  --display-name "${DISPLAY_NAME}" \
  --image-id "${IMAGE_OCID}" \
  --subnet-id "${SUBNET_OCID}" \
  --assign-public-ip true \
  --ssh-authorized-keys-file "${SSH_PUBLIC_KEY_PATH}" \
  --query "data.id" \
  --raw-output)

echo "==================================================================="
echo " SUCCESS! Instance provisioned successfully."
echo " Instance OCID: ${INSTANCE_OCID}"
echo "==================================================================="
