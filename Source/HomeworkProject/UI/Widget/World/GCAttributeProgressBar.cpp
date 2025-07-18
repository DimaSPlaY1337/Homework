// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/World/GCAttributeProgressBar.h"
#include "Components/ProgressBar.h"

void UGCAttributeProgressBar::SetProgressPercantage(float Percentage)
{
	HealthProgressBar->SetPercent(Percentage);
}
