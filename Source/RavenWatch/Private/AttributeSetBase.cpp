// Fill out your copyright notice in the Description page of Project Settings.


#include "AttributeSetBase.h"

void UAttributeSetBase::PostInitProperties()
{
	Super::PostInitProperties();

	for (TFieldIterator<FProperty> PropertyIterator(
		GetClass(),
		EFieldIteratorFlags::IncludeSuper);
		PropertyIterator;
		++PropertyIterator)
	{
		FStructProperty* StructProperty =
			CastField<FStructProperty>(*PropertyIterator);

		if (!StructProperty ||
			StructProperty->Struct != FAttribute::StaticStruct())
		{
			continue;
		}

		FAttribute* Attribute =
			StructProperty->ContainerPtrToValuePtr<FAttribute>(this);

		if (Attribute && !Attribute->IsValid())
		{
			Attribute->SetName(StructProperty->GetFName());
		}
	}
}

bool UAttributeSetBase::FindAttribute(
	FName AttributeName,
	FAttribute*& OutAttribute)
{
	OutAttribute = nullptr;

	if (AttributeName.IsNone())
	{
		return false;
	}

	for (TFieldIterator<FProperty> PropertyIterator(
		GetClass(),
		EFieldIteratorFlags::IncludeSuper);
		PropertyIterator;
		++PropertyIterator)
	{
		FStructProperty* StructProperty =
			CastField<FStructProperty>(*PropertyIterator);

		if (!StructProperty ||
			StructProperty->Struct != FAttribute::StaticStruct() ||
			StructProperty->GetFName() != AttributeName)
		{
			continue;
		}

		OutAttribute =
			StructProperty->ContainerPtrToValuePtr<FAttribute>(this);

		return OutAttribute != nullptr;
	}

	return false;
}

bool UAttributeSetBase::FindAttribute(
	FName AttributeName,
	const FAttribute*& OutAttribute) const
{
	OutAttribute = nullptr;

	if (AttributeName.IsNone())
	{
		return false;
	}

	for (TFieldIterator<FProperty> PropertyIterator(
		GetClass(),
		EFieldIteratorFlags::IncludeSuper);
		PropertyIterator;
		++PropertyIterator)
	{
		const FStructProperty* StructProperty =
			CastField<FStructProperty>(*PropertyIterator);

		if (!StructProperty ||
			StructProperty->Struct != FAttribute::StaticStruct() ||
			StructProperty->GetFName() != AttributeName)
		{
			continue;
		}

		OutAttribute =
			StructProperty->ContainerPtrToValuePtr<FAttribute>(this);

		return OutAttribute != nullptr;
	}

	return false;
}

