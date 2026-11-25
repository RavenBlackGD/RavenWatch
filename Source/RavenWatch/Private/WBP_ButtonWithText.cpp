// Fill out your copyright notice in the Description page of Project Settings.


#include "WBP_ButtonWithText.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UWBP_ButtonWithText::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Information)
	{
		Information->SetText(Info);
		Information->SetFont(InfoFont);
		Information->SetColorAndOpacity(InfoColor);
	}
}

void UWBP_ButtonWithText::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackgroundButton)
	{
		BackgroundButton->OnClicked.AddUniqueDynamic(this, &UWBP_ButtonWithText::HandleClickEvent);
	}
}

void UWBP_ButtonWithText::NativeDestruct()
{
	if (BackgroundButton)
	{
		BackgroundButton->OnClicked.RemoveDynamic(this, &UWBP_ButtonWithText::HandleClickEvent);
	}

	Super::NativeDestruct();
}

void UWBP_ButtonWithText::HandleClickEvent()
{
	WidgetClickResponceEvent.Broadcast();
}

void UWBP_ButtonWithText::FocusInnerButton()
{
	if (BackgroundButton)
	{
		BackgroundButton->SetKeyboardFocus();
	}
}